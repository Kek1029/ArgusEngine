//
// Created by etders on 05.04.2026.
//

#ifndef ARGUSENGINE_MODELMANAGER_HPP
#define ARGUSENGINE_MODELMANAGER_HPP
#include <sys/stat.h>

#include "FastBin.hpp"
#include "utils/TileFixedPoint.hpp"
#include "memory/FastIndexLinearAllocator.hpp"
#include "memory/Pair.hpp"
#define LOD_LEVEL 5


namespace ArgusEngine {
namespace Resources {
    // TODO: это пиздец....
    struct Mesh {
        uint32_t linkIndex;
        uint32_t size;
    };

    /*struct Point {
        alignas(8) Math::Vector3Fixed<24> offset;
        alignas(8) uint64_t tile_idx;
        alignas(8) uint64_t nextLayerPoint_idx;

        Point() = default;
    };*/

    //struct Point {
    //    Math::Vector3Fixed<24> offset;
    //    uint16_t materialID;
    //    uint8_t tile_offset;
    //     // 0-3 = child offset
    //     // 4 - 7 = flags
    //    uint8_t flags;
    //};

    /*
     * one layer all tree
     * [Parent1][Childrens1][Parent2][Childrens2]...
     *
     */

    struct Point {
        Math::vec3 point;
        int16_t tile;
        // 0-1 active child A, child B
        uint16_t flags;
    };

    /*
    * struct Link {
    size_t idx1, idx2;
    float restDist;
    float compliance;
    float lambda = 0.0f;
    float yieldDist;
    float fractureDist;
    float maxLimit;
    bool active = true;
};
     */

    struct Link {
        // point's idx

        uint32_t pointA;
        uint32_t pointB;

        // 0 - active
        uint8_t flags;

        Math::Fixed<24> dist;
        Math::Fixed<24> lambda;
        Math::Fixed<24> compliance;

        uint32_t pad1;
        uint32_t pad2;
    };

    struct Face {
        uint32_t a, b, c;
    };

    class ModelManager {
    private:
        FastBin::FileMapper& mapper;
        Memory::VirtualLinearAllocator<Mesh> meshes;

        Point* points;
        Link* links;
        Face* faces;

        uint32_t pSize, lSize, fSize;

        //uint32_t allocatedPoints = 0;
//
        //FORCE_INLINE uint32_t left_branch(uint32_t idx, uint8_t level) {
        //    return idx + 1;
        //}
//
        //FORCE_INLINE uint32_t right_branch(uint32_t idx, uint8_t level) {
        //    return (idx + (2 << level)) >> 1;
        //}
//
        //FORCE_INLINE uint16_t left_size(uint8_t level) {
        //    return (2 << level) >> 1;
        //}
//
        //FORCE_INLINE uint16_t right_size(uint8_t level) {
        //    return left_size(level);
        //}
//
        //FORCE_INLINE Math::vec3 getChildPointA(Math::vec3 pointA, Math::vec3 pointB) {
        //    Math::vec3 dir = pointB - pointA;
        //    return pointA + (dir >> 2u);
        //}
//
        //FORCE_INLINE Math::vec3 getChildPointB(Math::vec3 pointA, Math::vec3 pointB) {
        //    Math::vec3 dir = pointB - pointA;
        //    return pointB - (dir >> 2u);
        //}
//
        //FORCE_INLINE void allocNewBranch(uint8_t count = 1) {
        //    points.alloc(POINTS_PER_TREE + 1);
        //    ++allocatedPoints;
        //}

    public:
        ModelManager(FastBin::FileMapper& fm) : mapper(fm) {


        }

        //static constexpr uint32_t POINTS_PER_TREE = (1 << (LOD_LEVEL + 1)) - 1;

        void LoadScene(const char* path) {
            //TODO: load .agl(ArGus Local)
            //TODO: map points, links, faces, meshes
            //TODO: make hash table for id-mesh
        }

        //Mesh LoadModel(const char* path) {
        //    if (!mapper.map(path)) [[unlikely]] return Mesh();
//
        //    uint8_t* raw = (uint8_t*)mapper.data();
        //    if (std::memcpy(raw, "ARGUS", 5) != 0) return Mesh();
//
        //    pSize = *reinterpret_cast<uint32_t*>(raw + 5);
        //    lSize = *reinterpret_cast<uint32_t*>(raw + 9);
        //    fSize = *reinterpret_cast<uint32_t*>(raw + 13);
//
        //    Point* pData = reinterpret_cast<Point*>(raw + 17);
        //    links  = reinterpret_cast<Link*>(raw + 17 + (pSize * sizeof(Point)));
        //    faces  = reinterpret_cast<Face*>(raw + 17 + (lSize * sizeof(Face)));
//
//
        //}
    };

} // Resources
} // ArgusEngine

#endif //ARGUSENGINE_MODELMANAGER_HPP
