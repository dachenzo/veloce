#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <queue>
#include <vector>
#include <future>
#include <type_traits>

namespace veloce {
    class ThreadPool {

        using Task = std::move_only_function<void()>;
        std::size_t thread_cnt;
        std::queue<Task> work_queue;
        std::vector<std::thread> threads;
        std::mutex queue_lock;
        std::condition_variable cv;
        bool stopping = false;

        void work() {
            while (true) {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(queue_lock);
                    cv.wait(
                        lock,
                        [&](){
                            return stopping || !work_queue.empty();
                        }
                    );

                    if (stopping && work_queue.empty()) return;


                    task =  std::move(work_queue.front());
                    work_queue.pop();
                }
                task();
                
            }
        }

        void push_task(Task&& task) {
            {
                std::lock_guard<std::mutex> lock{queue_lock};

                if (stopping) {
                    throw std::runtime_error("cannot add a task to a stopped ThreadPool");
                }
                work_queue.push(std::move(task));
            }
            cv.notify_one();
        }

        public: 
        explicit ThreadPool(std::size_t num_threads): thread_cnt{num_threads} {
            threads.reserve(num_threads);
            for (std::size_t i = 0; i  < num_threads; i++) {
                threads.emplace_back([this]() {work();});
            }
        }

        explicit ThreadPool(): ThreadPool{4} {}

        ThreadPool(const ThreadPool& other) = delete;
        ThreadPool(ThreadPool&& other) = delete;

        ~ThreadPool() {
            {
                std::lock_guard<std::mutex> lock(queue_lock);
                stopping = true;
            }

            cv.notify_all();

            for (auto& t: threads) {
                if (t.joinable()) t.join();
            }
        }


        template<typename Func>
        std::future<std::invoke_result_t<Func>> submit(Func&& func) {
            using Result = std::invoke_result_t<Func>;
            auto task = std::packaged_task<Result()> (
                std::forward<Func>(func)
            );

            auto future = task.get_future();

            push_task(
                [task = std::move(task)]() mutable {
                    task();
                }
            );

            return future;
        }
        

        std::size_t worker_cnt() const {return thread_cnt;}

    };
};

