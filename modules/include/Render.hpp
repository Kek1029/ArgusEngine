#ifndef ARGUSENGINE_RENDER_HPP
#define ARGUSENGINE_RENDER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "../../engine/include/memory/LinearAllocator.hpp"


namespace ArgusEngine::Engine { class World; }

namespace ArgusEngine::Modules {
// TODO: Я СЮДА ДАЖЕ СМОТРЕТЬ НЕ ХОЧУ, ЭТО ПИЗДА, ЕБАНЫЙ MVP
/*class Render {
public:
    struct RenderInstanceData {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;
        glm::vec4 color;
        glm::vec4 extra;
    };

    struct DrawElementsIndirectCommand {
        uint32_t count;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t baseVertex;
        uint32_t baseInstance;
    };

    struct RenderContext {
        GLFWwindow* window;
        uint16_t width, height;
    };

    struct MeshEntry {
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t  baseVertex;
    };

    Render(const RenderContext& context, Engine::World* world, Systems::GPUBufferManager* bufferMgr);
    ~Render();

    void init();
    void render();
    void destroy();

    void updateCamera(float dt);
    uint32_t createMesh(float* vertices, size_t vSize, uint32_t* indices, size_t iSize);
    uint32_t loadMesh(const std::string& path);

    glm::vec3 getCameraPosition() const { return camPos; }
    glm::vec3 getCameraFront() const { return camFront; }
    RenderContext getContext() const { return ctx; }
private:
    RenderContext ctx;
    Engine::World* world;

    GLuint mainVAO = 0, globalVBO = 0, globalEBO = 0;
    GLuint instanceSSBO = 0, commandsSSBO = 0;

    struct Program {
        GLuint id;
        void use() const { glUseProgram(id); }
    } computeProg{}, renderProg{};

    struct { GLint uCount, uViewProj, uFrustum, uCamPos, uCamChunk; } locs{};

    uint32_t currentGpuCapacity = 0;
    size_t vboCap = 0, eboCap = 0;
    uint32_t vOffset = 0, iOffset = 0;
    uint32_t instanceCap = 0;

    struct MeshBatch {
        uint32_t meshID;
        uint32_t count;
        uint32_t offset;
    };

    GLuint atomicBuffer = 0;

    ArgusEngine::Memory::LinearAllocator<MeshEntry> meshEntries;
    ArgusEngine::Memory::LinearAllocator<DrawElementsIndirectCommand> commands;

    ArgusEngine::Memory::LinearAllocator<uint8_t> scratchBuffer;

    ArgusEngine::Systems::GPUBufferManager* gpuBufferMgr;

    glm::vec3 camPos{0, 15, 30}, camFront{0, 0, -1}, camUp{0, 1, 0}, camVel{0, 0, 0};
    float playerHeight = 1.8f;
    float yaw = -90.0f, pitch = 0.0f;
    bool firstMouse = true, isGrounded = false;
    double lastX = 0, lastY = 0;

    void checkAndResizeOutput(uint32_t needed);
    void checkAndResizeGeometry(size_t vCount, size_t iCount);
    GLuint compileShader(GLenum type, const char* src);
};*/

    // вот это блять уже похоже на человеческий код
    class Render {
    public:
        void StartFrame();

        void PushInstanceData(const void* data, size_t size);

        void Draw();

        void EndFrame();
    };

} // namespace ArgusEngine::Modules
#endif