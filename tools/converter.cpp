#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <chrono>

#include "FileMapper.hpp"
#include "memory/VirtualLinearAllocator.hpp"
#include "memory/Map.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using fixed = int32_t;

struct FinalPoint { fixed x, y, z; int16_t tile; uint16_t flags; };
struct FinalLink {
    uint32_t pointA, pointB;
    uint8_t flags, _pad[3];
    fixed dist, lambda, compliance;
    uint32_t pad1, pad2;
};
struct FinalFace { uint32_t a, b, c, pad; };

using FinalMesh = uint32_t;
//using FinalMesh = uint64_t;

//FORCE_INLINE FinalMesh makeMesh(uint32_t id, uint32_t offset) {
//    return (static_cast<uint64_t>(id) << 32) | offset;
//}

//FORCE_INLINE uint32_t getMeshId(FinalMesh mesh) {
//    return static_cast<uint32_t>(mesh >> 32);
//}
//
//FORCE_INLINE uint32_t getMeshOffset(FinalMesh mesh) {
//    return static_cast<uint32_t>(mesh);
//}

enum ObjLineType : uint8_t { Vertex = 1, TexCoord = 2, Normal = 3, Face = 4 };
struct alignas(8) ObjLine { uint32_t fixed_idx; uint32_t size; };

struct ParsedAST {
    ArgusEngine::Memory::VirtualLinearAllocator<fixed>* vals;
    ArgusEngine::Memory::VirtualLinearAllocator<ObjLine>* streams[5];

    ParsedAST() {
        vals = new ArgusEngine::Memory::VirtualLinearAllocator<fixed>();
        for(int i = 0; i < 5; ++i) streams[i] = new ArgusEngine::Memory::VirtualLinearAllocator<ObjLine>();
    }
} parsedAST;

struct SimpleImage {
    int w = 0, h = 0;
    std::vector<uint8_t> data;
    bool active = false;

    float getSample(float u, float v) {
        if (!active || data.empty()) return 0.0f;
        int x = std::clamp((int)(u * (w - 1)), 0, w - 1);
        int y = std::clamp((int)((1.0f - v) * (h - 1)), 0, h - 1);
        return data[y * w + x] / 255.0f;
    }
};

fixed parseDouble(const char* ptr, uint32_t size) {
    const char* end = ptr + size;
    long long int_part = 0, frac_part = 0, divisor = 1;
    bool negative = false;
    while (ptr < end && (unsigned char)*ptr <= 32) ptr++;
    if (ptr < end && *ptr == '-') { negative = true; ptr++; }
    while (ptr < end && *ptr >= '0' && *ptr <= '9') { int_part = int_part * 10 + (*ptr - '0'); ptr++; }
    if (ptr < end && *ptr == '.') {
        ptr++; int prec = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9' && prec < 8) {
            frac_part = frac_part * 10 + (*ptr - '0'); divisor *= 10; ptr++; prec++;
        }
    }
    fixed res = static_cast<fixed>(int_part << 24);
    if (frac_part > 0) res += static_cast<fixed>((frac_part << 24) / divisor);
    return negative ? -res : res;
}

void parseString(const char* ptr, uint32_t size, ObjLineType type) {
    const char* end = ptr + size;
    uint32_t startIdx = parsedAST.vals->size();
    uint32_t count = 0;
    const char* curr = ptr;
    while (curr < end) {
        while (curr < end && (unsigned char)*curr <= 32) curr++;
        if (curr >= end) break;
        if (type == Face) {
            int32_t v = atoi(curr) - 1;
            parsedAST.vals->push_back(v);
            while (curr < end && (unsigned char)*curr > 32) curr++;
            count++;
        } else {
            const char* ts = curr;
            while (curr < end && (unsigned char)*curr > 32) curr++;
            parsedAST.vals->push_back(parseDouble(ts, (uint32_t)(curr - ts)));
            count++;
        }
    }
    if (count > 0) parsedAST.streams[type]->push_back({startIdx, count});
}

class ArgusConverter {
    struct FullPoint { fixed x, y, z, nx, ny, nz; float u, v; };
    ArgusEngine::Memory::Map<uint64_t, uint32_t> vertexCache;
    std::map<uint64_t, std::vector<uint32_t>> edgePoints;
    std::vector<FinalPoint> finalPoints;
    std::vector<FinalLink> finalLinks;
    std::vector<FinalFace> finalFaces;
    SimpleImage dispMap;
    int subdiv;

public:
    ArgusConverter(int s) : vertexCache(65536), subdiv(s) {}

    void loadDisplacement(const char* path) {
        if (!path) return;

        int channels;
        unsigned char* img = stbi_load(path, &dispMap.w, &dispMap.h, &channels, 1); // Грузим только 1 канал (ч/б)

        if (!img) {
            printf("Failed to load image %s\n", path);
            return;
        }

        printf("Loaded displacement: %dx%d, channels: %d\n", dispMap.w, dispMap.h, channels);

        dispMap.data.assign(img, img + (dispMap.w * dispMap.h));
        dispMap.active = true;

        stbi_image_free(img);
    }

    FullPoint getFullPoint(uint32_t vIdx) {
        auto& vStream = (*parsedAST.streams[Vertex]);
        if (vIdx >= vStream.size()) return {0,0,0,0,(1<<24),0, 0.5f, 0.5f};

        uint32_t v_off = vStream[vIdx].fixed_idx;
        FullPoint p;
        p.x = (*parsedAST.vals)[v_off];
        p.y = (*parsedAST.vals)[v_off+1];
        p.z = (*parsedAST.vals)[v_off+2];

        if (vIdx < parsedAST.streams[Normal]->size()) {
            uint32_t n_off = (*parsedAST.streams[Normal])[vIdx].fixed_idx;
            p.nx = (*parsedAST.vals)[n_off];
            p.ny = (*parsedAST.vals)[n_off+1];
            p.nz = (*parsedAST.vals)[n_off+2];
        } else { p.nx = 0; p.ny = (1 << 24); p.nz = 0; }

        p.u = 0.5f; p.v = 0.5f;
        return p;
    }

    uint32_t addFinalPoint(FullPoint p) {
        if (dispMap.active && !dispMap.data.empty()) {
            float h = dispMap.getSample(p.u, p.v);
            fixed strength = (fixed)(h * (1 << 21));
            p.x += (fixed)(((int64_t)p.nx * strength) >> 24);
            p.y += (fixed)(((int64_t)p.ny * strength) >> 24);
            p.z += (fixed)(((int64_t)p.nz * strength) >> 24);
        }
        uint32_t idx = (uint32_t)finalPoints.size();
        finalPoints.push_back({p.x, p.y, p.z, 0, 0});
        return idx;
    }

    void createLink(uint32_t a, uint32_t b) {
        if (a == b) return;
        FinalLink l{};
        l.pointA = a; l.pointB = b; l.flags = 1;
        double dx = (double)(finalPoints[a].x - finalPoints[b].x) / (1 << 24);
        double dy = (double)(finalPoints[a].y - finalPoints[b].y) / (1 << 24);
        double dz = (double)(finalPoints[a].z - finalPoints[b].z) / (1 << 24);
        l.dist = (fixed)(sqrt(dx*dx + dy*dy + dz*dz) * (1 << 24));
        finalLinks.push_back(l);
    }

    std::vector<uint32_t> getSubdividedEdge(uint32_t vA, uint32_t vB) {
        uint64_t key = (uint64_t)std::min(vA, vB) << 32 | std::max(vA, vB);
        bool rev = vA > vB;
        if (edgePoints.count(key)) {
            auto res = edgePoints[key];
            if (rev) std::reverse(res.begin(), res.end());
            return res;
        }

        FullPoint a = getFullPoint(vA), b = getFullPoint(vB);
        std::vector<uint32_t> chain;

        uint32_t prev = addFinalPoint(a);
        chain.push_back(prev);

        for (int i = 1; i <= subdiv; ++i) {
            float t = (float)i / subdiv;
            FullPoint p;
            p.x = a.x + (fixed)((int64_t)(b.x - a.x) * t);
            p.y = a.y + (fixed)((int64_t)(b.y - a.y) * t);
            p.z = a.z + (fixed)((int64_t)(b.z - a.z) * t);
            p.nx = a.nx + (fixed)((int64_t)(b.nx - a.nx) * t);
            p.ny = a.ny + (fixed)((int64_t)(b.ny - a.ny) * t);
            p.nz = a.nz + (fixed)((int64_t)(b.nz - a.nz) * t);
            p.u = a.u + (b.u - a.u) * t;
            p.v = a.v + (b.v - a.v) * t;

            uint32_t curr = (i == subdiv) ? addFinalPoint(b) : addFinalPoint(p);
            createLink(prev, curr);
            chain.push_back(curr);
            prev = curr;
        }
        edgePoints[key] = rev ? std::vector<uint32_t>(chain.rbegin(), chain.rend()) : chain;
        return chain;
    }

    void process() {
        auto* faces = parsedAST.streams[Face];
        for (uint32_t i = 0; i < faces->size(); ++i) {
            ObjLine f = (*faces)[i];
            std::vector<uint32_t> faceVerts;
            fixed cx = 0, cy = 0, cz = 0, cnx = 0, cny = 0, cnz = 0;
            float cu = 0, cv = 0;

            for (uint32_t n = 0; n < f.size; ++n) {
                uint32_t vIdx = (*parsedAST.vals)[f.fixed_idx + n];
                faceVerts.push_back(vIdx);
                FullPoint p = getFullPoint(vIdx);
                cx += p.x / (fixed)f.size; cy += p.y / (fixed)f.size; cz += p.z / (fixed)f.size;
                cnx += p.nx / (fixed)f.size; cny += p.ny / (fixed)f.size; cnz += p.nz / (fixed)f.size;
                cu += p.u / f.size; cv += p.v / f.size;
            }

            uint32_t centerIdx = addFinalPoint({cx, cy, cz, cnx, cny, cnz, cu, cv});

            std::vector<std::vector<uint32_t>> outerEdges;
            for (size_t n = 0; n < faceVerts.size(); ++n) {
                outerEdges.push_back(getSubdividedEdge(faceVerts[n], faceVerts[(n+1)%faceVerts.size()]));
            }

            auto prevLayer = outerEdges;
            for (int l = 1; l < subdiv; ++l) {
                float t = 1.0f - (float)l / subdiv;
                std::vector<std::vector<uint32_t>> currLayer;
                for (size_t n = 0; n < outerEdges.size(); ++n) {
                    std::vector<uint32_t> edgeChain;
                    for (size_t s = 0; s < outerEdges[n].size(); ++s) {
                        FullPoint p;
                        uint32_t outIdx = outerEdges[n][s];
                        p.x = cx + (fixed)((int64_t)(finalPoints[outIdx].x - cx) * t);
                        p.y = cy + (fixed)((int64_t)(finalPoints[outIdx].y - cy) * t);
                        p.z = cz + (fixed)((int64_t)(finalPoints[outIdx].z - cz) * t);
                        p.nx = cnx; p.ny = cny; p.nz = cnz;
                        p.u = cu + (0.5f - cu) * (1.0f - t); // Очень грубый лерп UV
                        p.v = cv + (0.5f - cv) * (1.0f - t);

                        uint32_t inIdx = addFinalPoint(p);
                        edgeChain.push_back(inIdx);
                        createLink(inIdx, outIdx);
                    }
                    currLayer.push_back(edgeChain);
                }
                for (size_t n = 0; n < currLayer.size(); ++n) {
                    for (size_t s = 0; s < currLayer[n].size() - 1; ++s) {
                        finalFaces.push_back({prevLayer[n][s], currLayer[n][s+1], currLayer[n][s], 0});
                        finalFaces.push_back({prevLayer[n][s], prevLayer[n][s+1], currLayer[n][s+1], 0});
                    }
                }
                prevLayer = currLayer;
            }

            for(auto& edge : prevLayer) {
                for(size_t s = 0; s < edge.size() - 1; ++s) {
                    finalFaces.push_back({edge[s], edge[s+1], centerIdx, 0});
                    createLink(edge[s], centerIdx);
                }
            }
        }

        if (finalPoints.size() > 1) {
            for (size_t i = finalPoints.size() - 1; i > 0; --i) {
                finalPoints[i].x -= finalPoints[i-1].x;
                finalPoints[i].y -= finalPoints[i-1].y;
                finalPoints[i].z -= finalPoints[i-1].z;
            }
        }
    }

    void save(const char* path) {
        FILE* f = fopen(path, "wb");
        uint32_t ps = (uint32_t)finalPoints.size(), ls = (uint32_t)finalLinks.size(), fs = (uint32_t)finalFaces.size();
        fwrite("ARGUSM00", 8, 1, f);
        fwrite(&ps, 4, 1, f); fwrite(&ls, 4, 1, f); fwrite(&fs, 4, 1, f);
        fwrite(finalPoints.data(), sizeof(FinalPoint), ps, f);
        fwrite(finalLinks.data(), sizeof(FinalLink), ls, f);
        fwrite(finalFaces.data(), sizeof(FinalFace), fs, f);
        fclose(f);
    }
};

FastBin::FileMapper fm;

struct FileModel {
    uint8_t magic[8];
    uint32_t pSize, lSize, fSize;

    const FinalPoint* finalPoints = nullptr;
    const FinalLink* finalLinks = nullptr;
    const FinalFace* finalFaces = nullptr;

    FileModel(const void* rawPtr) noexcept {
        const uint8_t* raw = static_cast<const uint8_t*>(rawPtr);

        std::memcpy(magic, raw, 8);
        pSize = *reinterpret_cast<const uint32_t*>(raw + 8);
        lSize = *reinterpret_cast<const uint32_t*>(raw + 12);
        fSize = *reinterpret_cast<const uint32_t*>(raw + 16);

        finalPoints = reinterpret_cast<const FinalPoint*>(raw + 20);
        finalLinks  = reinterpret_cast<const FinalLink*>(finalPoints + pSize);
        finalFaces  = reinterpret_cast<const FinalFace*>(finalLinks + lSize);
    }
};

// [mashesCount][meshTable][mesh1][mesh2][mesh3][meshN]

void compressToState(int argc, char** argv) {
    ArgusEngine::Memory::VirtualLinearAllocator<char*> paths;
    ArgusEngine::Memory::VirtualLinearAllocator<FinalPoint> finalPoints;
    ArgusEngine::Memory::VirtualLinearAllocator<FinalLink> finalLinks;
    ArgusEngine::Memory::VirtualLinearAllocator<FinalFace> finalFaces;
    ArgusEngine::Memory::VirtualLinearAllocator<FinalMesh> finalMeshes;

    for (int i = 1; i < argc; ++i) {
        paths.push_back(argv[i]);
    }

    for (size_t n = 0; n < paths.size(); ++n) {
        if (!fm.map(paths[n])) [[unlikely]] {
            printf("Error to read %s ", paths[n]);
            continue;
        }
        auto m = FileModel(fm.data());

        void* raw = finalPoints.alloc(m.pSize);
        memcpy(raw, m.finalPoints, m.pSize * sizeof(FinalPoint));

        raw = finalLinks.alloc(m.lSize);
        memcpy(raw, m.finalLinks, m.lSize * sizeof(FinalLink));

        raw = finalFaces.alloc(m.fSize);
        memcpy(raw, m.finalFaces, m.fSize * sizeof(FinalFace));

        raw = finalMeshes.alloc(1);
        // TODO: mashes

        //fileModels.push_back(m);


    }

    printf("Compression not implemented yet. Keep working on it!\n");
}


#define CLR_RESET  "\033[0m"
#define CLR_BOLD   "\033[1m"
#define CLR_RED    "\033[31m"
#define CLR_GREEN  "\033[32m"
#define CLR_CYAN   "\033[36m"
#define CLR_YELLOW "\033[33m"
#define CLR_GRAY   "\033[90m"

void print_help(const char* progName) {
    printf(CLR_CYAN CLR_BOLD "Argus Engine Model Converter v1.1\n" CLR_RESET);
    printf("Usage:\n");
    printf("  %s m -i <input.obj> [options]\n\n", progName);
    printf("Options:\n");
    printf("  -i <file>    Input .obj file (required)\n");
    printf("  -d <file>    Displacement map (.png, .jpg)\n");
    printf("  -s <int>     Subdivision level (default: 1)\n");
    printf("  -o <file>    Output .arg file (default: model.arg)\n");
    printf("  -h           Show this help\n");
}

void convertmodel(int argc, char** argv) {
    std::string inputObj = "";
    std::string inputDisp = "";
    std::string outputFile = "model.arg";
    int subdiv = 1;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) inputObj = argv[++i];
        else if (arg == "-d" && i + 1 < argc) inputDisp = argv[++i];
        else if (arg == "-o" && i + 1 < argc) outputFile = argv[++i];
        else if (arg == "-s" && i + 1 < argc) subdiv = std::stoi(argv[++i]);
        else if (arg == "-h") { print_help(argv[0]); return; }
    }

    if (inputObj.empty()) {
        printf(CLR_RED "Error: Input OBJ file is required! Use -i <file>\n" CLR_RESET);
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    if (!fm.map(inputObj.c_str())) {
        printf(CLR_RED "Error: Could not open %s\n" CLR_RESET, inputObj.c_str());
        return;
    }

    printf(CLR_YELLOW "==> Parsing geometry: %s\n" CLR_RESET, inputObj.c_str());
    const char *p = (const char*)fm.data(), *e = p + fm.size();
    while (p < e) {
        while (p < e && (unsigned char)*p <= 32) p++;
        if (p >= e) break;
        const char* nextLine = p;
        while (nextLine < e && *nextLine != '\n' && *nextLine != '\r') nextLine++;

        if (*p == 'v' && *(p + 1) == ' ') parseString(p + 2, (uint32_t)(nextLine - p - 2), Vertex);
        else if (*p == 'v' && *(p + 1) == 'n') parseString(p + 3, (uint32_t)(nextLine - p - 3), Normal);
        else if (*p == 'f' && *(p + 1) == ' ') parseString(p + 2, (uint32_t)(nextLine - p - 2), Face);

        p = nextLine;
    }

    ArgusConverter conv(subdiv);
    if (!inputDisp.empty()) {
        printf(CLR_YELLOW "==> Loading displacement: %s\n" CLR_RESET, inputDisp.c_str());
        conv.loadDisplacement(inputDisp.c_str());
    }

    printf(CLR_YELLOW "==> Processing subdivision (level %d)...\n" CLR_RESET, subdiv);
    conv.process();

    printf(CLR_YELLOW "==> Saving to: %s\n" CLR_RESET, outputFile.c_str());
    conv.save(outputFile.c_str());

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    printf(CLR_GREEN "Done! Execution time: %.3f seconds\n" CLR_RESET, diff.count());
}

int main(int argc, char** argv) {
    struct Command {
        const char* desc;
        const char* usage;
        void (*run)(int, char**);
    };

    // стандартная мапа... пиздец, сжечь
    std::map<std::string, Command> registry = {
        {"m", {"Convert OBJ to Argus Model", "argus m -i <in.obj> [-s 2] [-d map.png] [-o out.arg]", convertmodel}},
        {"c", {"Compress State to binary",   "argus c -i <source> -o <dest>",                        compressToState}}
    };

    printf(CLR_CYAN CLR_BOLD "ARGUS CONVERTER\n" CLR_RESET);

    if (argc < 2 || registry.find(argv[1]) == registry.end()) {
        printf(CLR_BOLD "\nAvailable commands:\n" CLR_RESET);
        for (auto const& [name, cmd] : registry) {
            printf("  " CLR_GREEN "%-3s" CLR_RESET " %-30s " CLR_YELLOW "» %s\n" CLR_RESET,
                   name.c_str(), cmd.desc, cmd.usage);
        }
        printf("\n");
        return (argc < 2) ? 0 : 1;
    }

    registry[argv[1]].run(argc, argv);

    return 0;
}