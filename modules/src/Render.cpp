#include "Render.hpp"
/*#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstddef>

#include "DebugUI.hpp"
#include "../../engine/include/core/World.hpp"

#define CHUNK_SIZE_F 64.0f*/

//TODO: УДАЛИТЕ ЭТО БЛЯЯТЬ ОТСЮДА

namespace ArgusEngine::Modules {

/*const char* csSrc = R"(#version 430 core
layout(local_size_x = 64) in;

struct RawTransform {
    int x, y, z;
    float rx, ry, rz, rw;
    int sx, sy, sz;
    int tx, tz;
    uint flags, parent, layer;
};

struct Chunk {
    uint64_t bitmask;
    uint8_t generations[64];
    uint8_t _pad[56];
    RawTransform data[64];
};

struct RenderData { mat4 mat; mat4 norm; vec4 col; vec4 pad; };
struct DrawCmd    { uint count; uint instanceCount; uint firstIndex; uint baseVertex; uint baseInstance; };

layout(std430, binding = 0)  buffer PoolT   { Chunk chunks[]; };
layout(std430, binding = 5)  buffer Command { DrawCmd cmd; };
layout(std430, binding = 10) buffer OutBuf  { RenderData outputs[]; };

uniform uint uChunkCount;
uniform vec4 uFrustum[6];
uniform vec3 uCamPos;
uniform ivec2 uCamChunk;

bool isVisible(vec3 pos, float radius) {
    for(int i = 0; i < 6; i++) {
        if(dot(uFrustum[i].xyz, pos) + uFrustum[i].w < -radius) return false;
    }
    return true;
}

mat4 quatToMat4(vec4 q) {
    float x2 = q.x + q.x; float y2 = q.y + q.y; float z2 = q.z + q.z;
    float xx = q.x * x2;  float xy = q.x * y2;  float xz = q.x * z2;
    float yy = q.y * y2;  float yz = q.y * z2;  float zz = q.z * z2;
    float wx = q.w * x2;  float wy = q.w * y2;  float wz = q.w * z2;
    return mat4(
        vec4(1.0 - (yy + zz), xy + wz, xz - wy, 0.0),
        vec4(xy - wz, 1.0 - (xx + zz), yz + wx, 0.0),
        vec4(xz + wy, yz - wx, 1.0 - (xx + yy), 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

void main() {
    uint chunkIdx = gl_WorkGroupID.x;
    uint localIdx = gl_LocalInvocationIndex;

    if (!(chunks[chunkIdx].bitmask & (1ULL << localIdx))) return;

    RawTransform t = chunks[chunkIdx].data[localIdx];

    vec3 objLocalPos = vec3(float(t.x)/16777216.0, float(t.y)/16777216.0, float(t.z)/16777216.0);
    ivec2 deltaChunk = ivec2(t.tx - uCamChunk.x, t.tz - uCamChunk.y);

    vec3 relativePos = vec3(
        (float(deltaChunk.x) * 64.0) + (objLocalPos.x - uCamPos.x),
        objLocalPos.y - uCamPos.y,
        (float(deltaChunk.y) * 64.0) + (objLocalPos.z - uCamPos.z)
    );

    vec3 scale = vec3(float(t.sx)/16777216.0, float(t.sy)/16777216.0, float(t.sz)/16777216.0);
    float radius = max(scale.x, max(scale.y, scale.z)) * 1.5;

    if (isVisible(relativePos, radius)) {
        uint outIdx = atomicAdd(cmd.instanceCount, 1);

        mat4 model = quatToMat4(vec4(t.rx, t.ry, t.rz, t.rw));
        model[0] *= scale.x; model[1] *= scale.y; model[2] *= scale.z;
        model[3] = vec4(relativePos, 1.0);

        outputs[outIdx].mat = model;
        outputs[outIdx].norm = transpose(inverse(model));
        outputs[outIdx].col = vec4(1.0);
    }
})";

const char* vsSrc = R"(#version 460 core
#extension GL_ARB_shader_draw_parameters : require
layout(location=0) in vec3 aPos;
layout(location=2) in vec3 aNorm;
struct RenderData { mat4 mat; mat4 norm; vec4 col; vec4 pad; };
layout(std430, binding = 10) buffer OutBuf { RenderData data[]; };
uniform mat4 uViewProj;
out vec3 vNorm; out vec4 vCol;
void main() {
    uint gIdx = gl_InstanceID + gl_BaseInstanceARB;
    vCol = data[gIdx].col;
    vNorm = mat3(data[gIdx].norm) * aNorm;
    gl_Position = uViewProj * data[gIdx].mat * vec4(aPos, 1.0);
})";

const char* fsSrc = R"(#version 430 core
out vec4 fCol; in vec3 vNorm; in vec4 vCol;
void main() {
    vec3 n = normalize(vNorm);
    float diff = max(dot(n, normalize(vec3(0.5, 1.0, 0.3))), 0.0);
    fCol = vec4(vCol.rgb * (diff + 0.2), 1.0);
})";

struct Plane { glm::vec3 n; float d; };
void extractFrustum(const glm::mat4& vp, Plane* planes) {
    planes[0] = { glm::vec3(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0]), vp[3][3] + vp[3][0] };
    planes[1] = { glm::vec3(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0]), vp[3][3] - vp[3][0] };
    planes[2] = { glm::vec3(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1]), vp[3][3] + vp[3][1] };
    planes[3] = { glm::vec3(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1]), vp[3][3] - vp[3][1] };
    planes[4] = { glm::vec3(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2]), vp[3][3] + vp[3][2] };
    planes[5] = { glm::vec3(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2]), vp[3][3] - vp[3][2] };
    for (int i = 0; i < 6; i++) { float l = glm::length(planes[i].n); planes[i].n /= l; planes[i].d /= l; planes[i].d += 10.0f; }
}

bool isAABBVisible(const Plane* planes, glm::vec3 minAABB, glm::vec3 maxAABB) {
    for (int i = 0; i < 6; i++) {
        glm::vec3 p = minAABB;
        if (planes[i].n.x >= 0) p.x = maxAABB.x;
        if (planes[i].n.y >= 0) p.y = maxAABB.y;
        if (planes[i].n.z >= 0) p.z = maxAABB.z;
        if (glm::dot(planes[i].n, p) + planes[i].d < 0) return false;
    }
    return true;
}

Render::Render(const RenderContext& context, Engine::World* world, Systems::GPUBufferManager* bufferMgr)
    : ctx(context), world(world), gpuBufferMgr(bufferMgr), commands(256) {
    camPos = glm::vec3(0, 5, 10);
}

Render::~Render() { destroy(); }

void Render::init() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) std::cerr << "GLAD Fail" << std::endl;
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    auto link = [&](const char* v, const char* f, const char* c) {
        GLuint p = glCreateProgram();
        if(c) {
            GLuint cs = compileShader(GL_COMPUTE_SHADER, c);
            glAttachShader(p, cs); glLinkProgram(p); glDeleteShader(cs);
        } else {
            GLuint vs = compileShader(GL_VERTEX_SHADER, v);
            GLuint fs = compileShader(GL_FRAGMENT_SHADER, f);
            glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
            glDeleteShader(vs); glDeleteShader(fs);
        }
        return p;
    };

    computeProg.id = link(nullptr, nullptr, csSrc);
    renderProg.id  = link(vsSrc, fsSrc, nullptr);
    locs.uCount = glGetUniformLocation(computeProg.id, "uCount");
    locs.uViewProj = glGetUniformLocation(renderProg.id, "uViewProj");
    locs.uFrustum = glGetUniformLocation(computeProg.id, "uFrustum");
    locs.uCamChunk = glGetUniformLocation(computeProg.id, "uCamChunk");
    locs.uCamPos = glGetUniformLocation(computeProg.id, "uCamPos");

    glGenBuffers(1, &commandsSSBO);
    glGenVertexArrays(1, &mainVAO);
    checkAndResizeGeometry(100000, 100000);
    DebugUI::init(ctx.window);
}

void Render::render() {
    if (meshEntries.empty()) return;

    auto& poolT = world->registry().get_pool<Component_Transform>();
    auto& chunkStorage = poolT.get_raw();
    uint32_t chunkCount = static_cast<uint32_t>(chunkStorage.size());

    if (chunkCount == 0) return;

    size_t totalBytes = chunkCount * sizeof(Memory::Chunk<Component_Transform>);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuBufferMgr->get_buffer_id<Component_Transform>());
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalBytes, chunkStorage.data(), GL_STREAM_DRAW);

    glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)ctx.width/ctx.height, 0.1f, 10000.0f);
    glm::mat4 VP = proj * view;
    Plane planes[6]; extractFrustum(VP, planes);

    checkAndResizeOutput(chunkCount * 64);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandsSSBO);
    DrawElementsIndirectCommand initialCmd = {
        meshEntries[0].indexCount, 0, meshEntries[0].firstIndex, (uint32_t)meshEntries[0].baseVertex, 0
    };
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(initialCmd), &initialCmd);

    glUseProgram(computeProg.id);
    glUniform1ui(glGetUniformLocation(computeProg.id, "uChunkCount"), chunkCount);
    glUniform2i(locs.uCamChunk, (int)std::floor(camPos.x / 64.0f), (int)std::floor(camPos.z / 64.0f));
    glUniform3f(locs.uCamPos, std::fmod(camPos.x, 64.0f), camPos.y, std::fmod(camPos.z, 64.0f));

    glm::vec4 gpuPlanes[6];
    for(int i=0; i<6; i++) gpuPlanes[i] = glm::vec4(planes[i].n, planes[i].d);
    glUniform4fv(locs.uFrustum, 6, glm::value_ptr(gpuPlanes[0]));

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpuBufferMgr->get_buffer_id<Component_Transform>());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, commandsSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, instanceSSBO);

    glDispatchCompute(chunkCount, 1, 1);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderProg.id);
    glUniformMatrix4fv(locs.uViewProj, 1, GL_FALSE, glm::value_ptr(VP));

    glBindVertexArray(mainVAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandsSSBO);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);

    DebugUI::begin();
    DebugUI::render(0.016f, chunkCount * 64, 0);
    DebugUI::draw();
}

void Render::updateCamera(float dt) {
    double x, y; glfwGetCursorPos(ctx.window, &x, &y);
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    yaw += (float)(x - lastX) * 0.1f; pitch = std::clamp(pitch + (float)(lastY - y) * 0.1f, -89.0f, 89.0f);
    lastX = x; lastY = y;

    camFront = glm::normalize(glm::vec3(cos(glm::radians(yaw))*cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw))*cos(glm::radians(pitch))));
    float s = (glfwGetKey(ctx.window, GLFW_KEY_LEFT_SHIFT)) ? 50.0f : 20.0f;
    if(glfwGetKey(ctx.window, GLFW_KEY_W)) camPos += camFront * s * dt;
    if(glfwGetKey(ctx.window, GLFW_KEY_S)) camPos -= camFront * s * dt;

    if (glfwGetMouseButton(ctx.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        world->update_systems<Component_Transform, Component_PhysicsBody>(
            dt, [this](auto&& trans, Component_PhysicsBody& phys, float) {
                if (glm::distance(trans.get(), this->camPos) < 15.0f) {
                    phys.template Set<0>(Math::Vector3Fixed<24>(camFront.x*100, camFront.y*100, camFront.z*100));
                }
            }
        );
    }
}

void Render::checkAndResizeOutput(uint32_t count) {
    if (count <= instanceCap && instanceSSBO != 0) return;
    instanceCap = count + 1024;
    if (instanceSSBO) glDeleteBuffers(1, &instanceSSBO);
    glGenBuffers(1, &instanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, instanceCap * 160, nullptr, GL_STREAM_DRAW);
}

uint32_t Render::createMesh(float* v, size_t vs, uint32_t* idx, size_t is) {
    uint32_t id = (uint32_t)meshEntries.size();
    uint32_t vc = vs/32, ic = is/4;
    checkAndResizeGeometry(vc, ic);
    glBindBuffer(GL_ARRAY_BUFFER, globalVBO); glBufferSubData(GL_ARRAY_BUFFER, vOffset*32, vs, v);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalEBO); glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iOffset*4, is, idx);
    meshEntries.push_back({ic, iOffset, (int32_t)vOffset});
    vOffset += vc; iOffset += ic;
    return id;
}

uint32_t Render::loadMesh(const std::string& path) {
    //Assimp::Importer imp;
    //const aiScene* s = imp.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals);
    //if(!s) return 0;
    //aiMesh* m = s->mMeshes[0];
    //std::vector<float> v; std::vector<uint32_t> ind;
    //for(uint32_t i=0; i<m->mNumVertices; i++) {
    //    v.push_back(m->mVertices[i].x); v.push_back(m->mVertices[i].y); v.push_back(m->mVertices[i].z);
    //    v.push_back(0); v.push_back(0); // UV
    //    v.push_back(m->mNormals[i].x); v.push_back(m->mNormals[i].y); v.push_back(m->mNormals[i].z);
    //}
    //for(uint32_t i=0; i<m->mNumFaces; i++) for(uint32_t j=0; j<3; j++) ind.push_back(m->mFaces[i].mIndices[j]);
    //return createMesh(v.data(), v.size()*4, ind.data(), ind.size()*4);

    return 0;
}

void Render::checkAndResizeGeometry(size_t v, size_t i) {
    if (vOffset + v <= vboCap && iOffset + i <= eboCap && globalVBO != 0) return;
    vboCap = (vOffset+v+1000)*2; eboCap = (iOffset+i+1000)*2;
    glGenBuffers(1, &globalVBO); glBindBuffer(GL_ARRAY_BUFFER, globalVBO); glBufferData(GL_ARRAY_BUFFER, vboCap*32, nullptr, GL_STATIC_DRAW);
    glGenBuffers(1, &globalEBO); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalEBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboCap*4, nullptr, GL_STATIC_DRAW);
    glBindVertexArray(mainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, globalVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, 0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, (void*)20); glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalEBO);
}

GLuint Render::compileShader(GLenum t, const char* s) {
    GLuint sh = glCreateShader(t); glShaderSource(sh, 1, &s, 0); glCompileShader(sh);
    return sh;
}

void Render::destroy() {
    DebugUI::shutdown();
    glDeleteBuffers(1, &instanceSSBO); glDeleteBuffers(1, &commandsSSBO);
    glDeleteBuffers(1, &globalVBO); glDeleteBuffers(1, &globalEBO); glDeleteVertexArrays(1, &mainVAO);
}*/


    // TODO: сделать реализацию, круто, да?
void Render::StartFrame() {

}

void Render::PushInstanceData(const void *data, size_t size) {

}

void Render::EndFrame() {

}

void Render::Draw() {

}

} // namespace