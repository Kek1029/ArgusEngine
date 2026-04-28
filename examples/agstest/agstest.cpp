// TODO: переделать все под fast bin

#include <cmath>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../deps/FastBin/include/FileMapper.hpp"

#pragma pack(push, 1)
struct FinalPoint { int32_t x, y, z; int16_t tile; uint16_t flags; };
struct FinalLink { uint32_t pointA, pointB; uint8_t flags, _pad[3]; int32_t dist, lambda, compliance; uint32_t pad1, pad2; };
struct FinalFace { uint32_t a, b, c, pad; };
#pragma pack(pop)

struct Camera {
    float x = 0, y = 1, z = 5;
    float yaw = -90.0f, pitch = 0;
    float speed = 0.1f, sensitivity = 0.15f;
} cam;

struct SceneData {
    std::vector<float> absX, absY, absZ;
    uint32_t pSize = 0, lSize = 0, fSize = 0;
    FinalLink* links = nullptr;
    FinalFace* faces = nullptr;
    FastBin::FileMapper mapper;
    int grabbedIdx = -1;
    double lastGrabZ = 0;
    bool rmb = false;
    double lx, ly;
} scene;

// Вспомогательная функция для замены gluUnProject
glm::vec3 unproject(glm::vec3 win, glm::mat4 model, glm::mat4 proj, glm::vec4 viewport) {
    return glm::unProject(win, model, proj, viewport);
}

// Вспомогательная функция для замены gluProject
glm::vec3 project(glm::vec3 obj, glm::mat4 model, glm::mat4 proj, glm::vec4 viewport) {
    return glm::project(obj, model, proj, viewport);
}

void loadToScene(const char* filename) {
    if (!scene.mapper.map(filename)) return;
    uint8_t* raw = (uint8_t*)scene.mapper.data();
    if (std::memcmp(raw, "ARGUSM00", 8) != 0) {
        printf("Invalid format\n");
        return;
    }

    scene.pSize = *reinterpret_cast<uint32_t*>(raw + 8);
    scene.lSize = *reinterpret_cast<uint32_t*>(raw + 12);
    scene.fSize = *reinterpret_cast<uint32_t*>(raw + 16);

    FinalPoint* pData = reinterpret_cast<FinalPoint*>(raw + 20);

    scene.links = reinterpret_cast<FinalLink*>(pData + scene.pSize);
    scene.faces = reinterpret_cast<FinalFace*>(scene.links + scene.lSize);

    double invQ = 1.0 / (1 << 24);
    double cx = 0, cy = 0, cz = 0;
    scene.absX.assign(scene.pSize, 0);
    scene.absY.assign(scene.pSize, 0);
    scene.absZ.assign(scene.pSize, 0);

    for (uint32_t i = 0; i < scene.pSize; ++i) {
        cx += (double)pData[i].x * invQ;
        cy += (double)pData[i].y * invQ;
        cz += (double)pData[i].z * invQ;
        scene.absX[i] = (float)cx;
        scene.absY[i] = (float)cy;
        scene.absZ[i] = (float)cz;
    }
}

void processInput(GLFWwindow* window) {
    float fx = cosf(glm::radians(cam.yaw)), fz = sinf(glm::radians(cam.yaw));
    float rx = -sinf(glm::radians(cam.yaw)), rz = cosf(glm::radians(cam.yaw));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { cam.x += fx * cam.speed; cam.z += fz * cam.speed; }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { cam.x -= fx * cam.speed; cam.z -= fz * cam.speed; }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { cam.x -= rx * cam.speed; cam.z -= rz * cam.speed; }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { cam.x += rx * cam.speed; cam.z += rz * cam.speed; }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cam.y += cam.speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam.y -= cam.speed;
}

void display(GLFWwindow* window) {
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int w, h; glfwGetFramebufferSize(window, &w, &h);
    float aspect = (float)w / (float)h;

    // Матрица проекции
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));

    // Матрица вида
    glm::mat4 view = glm::lookAt(
        glm::vec3(cam.x, cam.y, cam.z),
        glm::vec3(cam.x + cosf(glm::radians(cam.yaw)) * cosf(glm::radians(cam.pitch)),
                  cam.y + sinf(glm::radians(cam.pitch)),
                  cam.z + sinf(glm::radians(cam.yaw)) * cosf(glm::radians(cam.pitch))),
        glm::vec3(0, 1, 0)
    );
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));

    // Логика захвата
    if (scene.grabbedIdx != -1) {
        double mx, my; glfwGetCursorPos(window, &mx, &my);
        glm::vec4 viewport(0, 0, w, h);
        glm::vec3 winPos(mx, (double)h - my, scene.lastGrabZ);
        glm::vec3 worldPos = glm::unProject(winPos, view, proj, viewport);

        scene.absX[scene.grabbedIdx] = worldPos.x;
        scene.absY[scene.grabbedIdx] = worldPos.y;
        scene.absZ[scene.grabbedIdx] = worldPos.z;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);

    glBegin(GL_TRIANGLES); glColor3f(0.6f, 0.6f, 0.7f);
    for (uint32_t i = 0; i < scene.fSize; ++i) {
        uint32_t a = scene.faces[i].a, b = scene.faces[i].b, c = scene.faces[i].c;
        if(a >= scene.pSize || b >= scene.pSize || c >= scene.pSize) continue;

        glm::vec3 pA(scene.absX[a], scene.absY[a], scene.absZ[a]);
        glm::vec3 pB(scene.absX[b], scene.absY[b], scene.absZ[b]);
        glm::vec3 pC(scene.absX[c], scene.absY[c], scene.absZ[c]);

        glm::vec3 normal = glm::normalize(glm::cross(pB - pA, pC - pA));
        glNormal3f(normal.x, normal.y, normal.z);

        glVertex3f(pA.x, pA.y, pA.z);
        glVertex3f(pB.x, pB.y, pB.z);
        glVertex3f(pC.x, pC.y, pC.z);
    }
    glEnd();

    glDisable(GL_LIGHTING); glPointSize(4.0f);
    glBegin(GL_POINTS); glColor3f(1.0f, 0.2f, 0.2f);
    for(uint32_t i=0; i<scene.pSize; i++) glVertex3f(scene.absX[i], scene.absY[i], scene.absZ[i]);
    glEnd();
}

int main(int argc, char** argv) {
    if (argc < 2 || !glfwInit()) return 1;
    GLFWwindow* w = glfwCreateWindow(1280, 720, "Argus Engine - Debug View", NULL, NULL);
    if(!w) return 1;
    glfwMakeContextCurrent(w); gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetMouseButtonCallback(w, [](GLFWwindow* win, int b, int a, int m){
        if (b == GLFW_MOUSE_BUTTON_LEFT) {
            if (a == GLFW_PRESS) {
                double mx, my; glfwGetCursorPos(win, &mx, &my);
                int width, height; glfwGetFramebufferSize(win, &width, &height);

                // Генерим матрицы вручную для теста пересечения
                float aspect = (float)width / (float)height;
                glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
                glm::mat4 view = glm::lookAt(
                    glm::vec3(cam.x, cam.y, cam.z),
                    glm::vec3(cam.x + cosf(glm::radians(cam.yaw)) * cosf(glm::radians(cam.pitch)),
                              cam.y + sinf(glm::radians(cam.pitch)),
                              cam.z + sinf(glm::radians(cam.yaw)) * cosf(glm::radians(cam.pitch))),
                    glm::vec3(0, 1, 0)
                );
                glm::vec4 viewport(0, 0, width, height);

                int best = -1; float minD = 15.0f;
                for (uint32_t i = 0; i < scene.pSize; ++i) {
                    glm::vec3 objPos(scene.absX[i], scene.absY[i], scene.absZ[i]);
                    glm::vec3 winPos = glm::project(objPos, view, proj, viewport);
                    float d = hypotf((float)winPos.x - (float)mx, (float)(height - winPos.y) - (float)my);
                    if (d < minD) { minD = d; best = i; scene.lastGrabZ = winPos.z; }
                }
                scene.grabbedIdx = best;
            } else scene.grabbedIdx = -1;
        }
        if (b == GLFW_MOUSE_BUTTON_RIGHT) {
            scene.rmb = (a == GLFW_PRESS);
            glfwSetInputMode(win, GLFW_CURSOR, scene.rmb ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(win, &scene.lx, &scene.ly);
        }
    });

    loadToScene(argv[1]);

    while (!glfwWindowShouldClose(w)) {
        processInput(w);
        if(scene.rmb) {
            double mx, my; glfwGetCursorPos(w, &mx, &my);
            cam.yaw += (float)(mx - scene.lx)*cam.sensitivity;
            cam.pitch -= (float)(my - scene.ly)*cam.sensitivity;
            if(cam.pitch > 89) cam.pitch = 89; if(cam.pitch < -89) cam.pitch = -89;
            scene.lx = mx; scene.ly = my;
        }
        display(w); glfwSwapBuffers(w); glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}