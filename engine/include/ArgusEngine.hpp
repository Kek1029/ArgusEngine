//
// Created by etders on 24.03.2026.
//

#ifndef ARGUSENGINE_ARGUSENGINE_HPP
#define ARGUSENGINE_ARGUSENGINE_HPP

#include "core/Engine.hpp"

namespace ArgusEngine {

    class Window {
    public:
        Window(int w, int h, const char* title) {
            if (!glfwInit()) std::cerr << "Failed to initialize GLFW" << std::endl;

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            handle = glfwCreateWindow(w, h, title, nullptr, nullptr);
            if (!handle) {
                const char* description;
                int code = glfwGetError(&description);
                if (description) {
                    std::cerr << "GLFW Error during creation (" << code << "): " << description << std::endl;
                }
                std::cerr << "glfwCreateWindow() failed " << std::endl;
            }

            glfwMakeContextCurrent(handle);
            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                std::cerr << "Failed to initialize GLAD" << std::endl;
            }

            //glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
////
            //if (glfwRawMouseMotionSupported()) {
            //    glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            //}
            glfwSwapInterval(0);
        }

        ~Window() {
            glfwDestroyWindow(handle);
            glfwTerminate();
        }

        bool shouldClose() const { return glfwWindowShouldClose(handle); }
        void swapBuffers() { glfwSwapBuffers(handle); }
        void pollEvents() { glfwPollEvents(); }
        GLFWwindow* getNative() { return handle; }

    private:
        GLFWwindow* handle;
    };

    class Application {
    private:

        const uint16_t WIDTH = 1280;
        const uint16_t HEIGHT = 720;

        Window window;
        Engine::Engine app;

        Engine::Timer timer;

    public:

        Application()
    : window(WIDTH, HEIGHT, "Argus Engine")
      //app(Modules::Render::RenderContext(
      //    window.getNative(),
      //    WIDTH, HEIGHT))
        {}

        ~Application() {
            app.destroy();
        }

        int run() {
            if (!window.getNative()) return -1;

            app.start();

            while (!window.shouldClose()) {
                timer.Update();

                window.pollEvents();

                app.update(timer.DeltaSec());

                while (timer.ShouldFixedStep()) {
                    app.fixedUpdate();
                }

                app.render();

                window.swapBuffers();

            }

            return 0;
        }
    };

} // ArgusEngine

#endif //ARGUSENGINE_ARGUSENGINE_HPP