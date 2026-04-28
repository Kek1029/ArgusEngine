#ifndef ARGUSENGINE_TIMER_HPP
#define ARGUSENGINE_TIMER_HPP

#include <chrono>
#include <x86intrin.h>
#include <cstdint>
#include <thread>

namespace ArgusEngine::Engine {
    constexpr double FIXED_TIME = 1.0 / 60.0;
    class Timer {
        // покумекать о переделке таймера на фиксед поинт, ибо не дело
    public:
        Timer() {
            calibrate();
            startCycles = __rdtsc();
            lastCycles = startCycles;
        }

        void Update() {
            uint64_t now = __rdtsc();
            uint64_t elapsed = now - lastCycles;
            lastCycles = now;

            double dt = static_cast<double>(elapsed) * invFrequency;

            if (dt > 0.1) dt = 0.016;

            deltaSec = dt;
            totalSec += dt;
            accumulator += dt;
            frameCount++;

            if (dt > 0) {
                smoothedFps = smoothedFps * 0.95 + (1.0 / dt) * 0.05;
            }
        }

        bool ShouldFixedStep() {

            if (accumulator >= FIXED_TIME) {
                accumulator -= FIXED_TIME;
                return true;
            }
            return false;
        }

        double DeltaSec() const { return deltaSec; }
        double TotalSec() const { return totalSec; }
        double FPS() const { return smoothedFps; }

    private:
        void calibrate() {
            auto t1 = std::chrono::steady_clock::now();
            uint64_t r1 = __rdtsc();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto t2 = std::chrono::steady_clock::now();
            uint64_t r2 = __rdtsc();

            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
            double ticksPerNs = static_cast<double>(r2 - r1) / static_cast<double>(ns);
            invFrequency = 1.0 / (ticksPerNs * 1e9);
        }

        uint64_t startCycles;
        uint64_t lastCycles;
        double invFrequency = 0.0;
        double deltaSec = 0.0;
        double totalSec = 0.0;
        double accumulator = 0.0;
        uint64_t frameCount = 0;
        double smoothedFps = 0.0;
    };
}

#endif