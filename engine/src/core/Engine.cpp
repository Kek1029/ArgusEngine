#include "core/Engine.hpp"

namespace ArgusEngine::Engine {

    // TODO: это пиздец вдвойне... под чем я это писал?

/*constexpr size_t MAX_SPAWN_PER_FRAME = 1024;
struct SpawnRequest {
    glm::vec3 position;
    glm::vec3 velocity;
};
struct InputState {
    glm::vec2 mousePos;
    bool mouseLeft;
    bool mouseRight;
    // uint64_t keysMask;
};

static InputState currentInput;
static SpawnRequest spawnRingBuffer[MAX_SPAWN_PER_FRAME];
static size_t spawnCount = 0;

struct FrameStats {
    double logicTimeMs = 0;
} lastStats;

Engine::Engine(const Modules::Render::RenderContext &context)
    : rend(context, &world, &gpuMgr) {}

Engine::~Engine() {}

void Engine::internalSpawn(const glm::vec3& pos, const glm::vec3& vel) {
    int tx = static_cast<int>(std::floor(pos.x / 64.0f));
    int tz = static_cast<int>(std::floor(pos.z / 64.0f));

    auto monkey = world.create_entity();

    Component_Transform t;
    t.template Set<1>(Math::Vector3Fixed<24>(pos.x - (tx * 64.0f), pos.y, pos.z - (tz * 64.0f)));
    t.template Set<0>(Math::QuatFixed());
    t.template Set<3>(Math::Vector3Fixed<24>(1.0f, 1.0f, 1.0f)); // Поставили индекс 3 (Масштаб)
    t.template Set<5>(tx);
    t.template Set<6>(tz);

    Component_PhysicsBody p;
    p.template Set<0>(Math::Vector3Fixed<24>(vel.x, vel.y, vel.z));
    p.template Set<2>(Math::Fixed32(1.0f));

    world.at(monkey)
        .add<Component_Transform>(t)
        .add<Component_PhysicsBody>(p);
}

void Engine::start() {
    rend.init();
    this->cachedModelID = rend.loadMesh("./Untitled.obj");

    for (int i = 0; i < 1'000'000; ++i) {
        internalSpawn({(i % 20) * 4.0f, 40.0f + (i * 0.01f), (i / 20) * 4.0f}, {0, 0, 0});
    }

    world.sync();
    rend.updateCamera(0.016f);
}

static bool isPushing = false;
static glm::vec3 pushSource = {0,0,0};

    void Engine::update() {
        timer.Update();
        float dt = static_cast<float>(timer.DeltaSec());

        double mx, my;
        glfwGetCursorPos(rend.getContext().window, &mx, &my);
        currentInput.mousePos = {static_cast<float>(mx), static_cast<float>(my)};
        currentInput.mouseLeft = glfwGetMouseButton(rend.getContext().window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        currentInput.mouseRight = glfwGetMouseButton(rend.getContext().window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        //rend.updateCamera(dt);
        glm::vec3 cameraPos = rend.getCameraPosition();
        glm::vec3 cameraFront = rend.getCameraFront();

        if (currentInput.mouseRight) {
            if (spawnCount < MAX_SPAWN_PER_FRAME) {
                glm::vec3 jitter = glm::vec3((rand() % 100 - 50) * 0.01f);
                spawnRingBuffer[spawnCount++] = {
                    cameraPos + cameraFront * 3.0f + jitter,
                    cameraFront * 60.0f
                };
            }
        }

        isPushing = currentInput.mouseLeft;
        if (isPushing) pushSource = cameraPos;

        while (timer.ShouldFixedStep()) {
            this->fixedUpdate(static_cast<float>(FIXED_TIME));
        }
    }


void Engine::fixedUpdate(float dt) {
    if (spawnCount > 0) {
        for (size_t i = 0; i < spawnCount; ++i) {
            internalSpawn(spawnRingBuffer[i].position, spawnRingBuffer[i].velocity);
        }
        spawnCount = 0;
    }

    auto t0 = timer.TotalSec();

    world.update_systems<Component_Transform, Component_PhysicsBody>(
        dt,
        [](auto&& trans, Component_PhysicsBody& phys, float currentDt) {

            glm::vec3 pos = trans.get();
            auto rawVel = phys.template Get<0>();
            glm::vec3 vel(rawVel.x.to_float(), rawVel.y.to_float(), rawVel.z.to_float());

            vel.y -= 9.8f * currentDt; // Гравитация
            pos += vel * currentDt;    // Интеграция позиции

            if (pos.y < 0.0f) {
                pos.y = 0.0f;
                vel.y *= -0.8f; // Потеря энергии при ударе
            }

            phys.template Set<0>(Math::Vector3Fixed<24>(vel.x, vel.y, vel.z));
            trans.set(pos);
        }
    );

    auto t1 = timer.TotalSec();
    lastStats.logicTimeMs = t1-t0;


        world.sync();
}

void Engine::render() {}
void Engine::destroy() {}
void Engine::printStats() {}*/

    Engine::Engine() {

    }

    void Engine::start() {

    }

    void Engine::update(float dt) {

    }

    void Engine::fixedUpdate() {

    }

    void Engine::render() {

    }

    void Engine::destroy() {

    }

} // namespace ArgusEngine::Engine