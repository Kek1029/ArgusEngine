//
// Created by etders on 26.03.2026.
//

#ifndef ARGUSENGINE_JOBSYSTEM_HPP
#define ARGUSENGINE_JOBSYSTEM_HPP

#include <atomic>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>

// TODO: переделать на нормальную lock free очередь
namespace ArgusEngine {
    namespace Multithread {
        using JobFunc = void (*)(void* data, uint32_t start, uint32_t end);

    struct Job {
        JobFunc function = nullptr;
        void* data = nullptr;
        uint32_t start = 0;
        uint32_t end = 0;
        std::atomic<int>* counter = nullptr;
    };

        class SafeQueue {
            std::deque<Job> jobs;
            std::mutex mtx;
            std::condition_variable cv;
        public:
            void Push(const Job& job) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    jobs.push_back(job);
                }
                cv.notify_one();
            }

            bool WaitAndPop(Job& outJob, std::atomic<bool>& running) {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this, &running] {
                    return !jobs.empty() || !running.load();
                });

                if (jobs.empty()) return false;

                outJob = jobs.front();
                jobs.pop_front();
                return true;
            }

            bool TryPop(Job& outJob) {
                std::lock_guard<std::mutex> lock(mtx);
                if (jobs.empty()) return false;
                outJob = jobs.front();
                jobs.pop_front();
                return true;
            }

            void NotifyAll() { cv.notify_all(); }
        };

    class JobSystem {
        struct alignas(128) WorkerData {
            std::thread thread;
            uint32_t id = 0;
        };

        std::vector<WorkerData*> workers;
        SafeQueue global_queue;
        std::atomic<bool> running{false};

    public:
        JobSystem() = default;
        ~JobSystem() { Shutdown(); }

        void Initialize(uint32_t thread_count = 0) {
            if (running.load()) return;
            uint32_t cores = (thread_count == 0) ? std::thread::hardware_concurrency() : thread_count;
            uint32_t num_workers = std::max(1u, cores);
            running.store(true);

            for (uint32_t i = 0; i < num_workers; ++i) {
                auto* w = new WorkerData();
                w->id = i;
                w->thread = std::thread(&JobSystem::WorkerLoop, this);
                workers.push_back(w);
            }
        }

        void Shutdown() {
            if (!running.load()) return;
            running.store(false);
            global_queue.NotifyAll();
            for (auto* w : workers) {
                if (w->thread.joinable()) w->thread.join();
                delete w;
            }
            workers.clear();
        }

        void Dispatch(uint32_t total_count, uint32_t batch_size, JobFunc func, void* data) {
            std::atomic<int> tasks_left{0};

            for (uint32_t i = 0; i < total_count; i += batch_size) {
                uint32_t end = std::min(i + batch_size, total_count);
                tasks_left.fetch_add(1, std::memory_order_relaxed);
                Job job = { func, data, i, end, &tasks_left };

                global_queue.Push(job);
            }

            Job job;
            while (tasks_left.load(std::memory_order_acquire) > 0) {
                if (global_queue.TryPop(job)) {
                    job.function(job.data, job.start, job.end);
                    job.counter->fetch_sub(1, std::memory_order_release);
                } else {
                    std::this_thread::yield();
                }
            }
        }

    private:
        void WorkerLoop() {
            while (running.load(std::memory_order_relaxed)) {
                Job job;
                if (global_queue.WaitAndPop(job, running)) {
                    job.function(job.data, job.start, job.end);
                    job.counter->fetch_sub(1, std::memory_order_release);
                }
            }
        }
    };
    }
}

#endif //ARGUSENGINE_JOBSYSTEM_HPP