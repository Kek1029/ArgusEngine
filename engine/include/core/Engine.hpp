//
// Created by etders on 26.03.2026.
//

#ifndef ARGUSENGINE_ENGINE_HPP
#define ARGUSENGINE_ENGINE_HPP
#include "Timer.hpp"
#include "World.hpp"
#include "jobSystem/JobSystem.hpp"
#include "Render.hpp"

namespace ArgusEngine {
    namespace Engine {
        class Engine {
        private:
            World world;
            //Multithread::JobSystem jb;
            Modules::Render rend;

        public:
            //Engine(const Modules::Render::RenderContext& context);

            Engine();
            ~Engine() = default;

            void start();

            void fixedUpdate();

            void update(float dt);

            void render();

            void destroy();
        };
    } // Engine
} // ArgusEngine

#endif //ARGUSENGINE_ENGINE_HPP