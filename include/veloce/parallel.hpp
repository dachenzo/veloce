#pragma once
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <exception>
#include <span>
#include <type_traits>
#include <vector>
#include <future>
#include <veloce/thread_pool.hpp>
#include <stdexcept>


namespace veloce {

    template<typename Func, typename T>
    concept VoidCopyableFunctionOn = std::invocable<Func, T&> && std::same_as<std::invoke_result_t<Func, T&>, void> && std::copy_constructible<std::remove_reference_t<Func>>;

    
    class Parallel {
        ThreadPool& pool;

        public:
        Parallel(ThreadPool& pool): pool{pool} {}
        Parallel(const Parallel& pool) = delete;
        Parallel(Parallel&& pool) = delete;

        template<typename T, VoidCopyableFunctionOn<T> Func>
        void operator()(std::span<T> data, Func&& func) {
            auto n = data.size();
            auto worker_cnt = std::min(n, pool.worker_cnt());

            if (n == 0) {
                return;
            }

            if (worker_cnt == 0) {
                throw std::runtime_error("thread pool has no workers");
            }

            auto step = n / worker_cnt;
            auto extra = n % worker_cnt;
            std::vector<std::future<std::invoke_result_t<Func, T&>>> chunk_results;
            chunk_results.reserve(worker_cnt); 
            
            std::exception_ptr first_error;

        
            auto increment = [&extra, &step](std::size_t i, bool mut) {
                if (extra > 0) {
                    extra-= mut ? 1 : 0;
                    return i+1+step;
                }
                return i+step;
            };

            for (std::size_t i = 0; i < n; i =  increment(i, true)) {
                auto stop = std::min(n, increment(i, false));
                chunk_results.push_back(
                        pool.submit( [data, i, stop, func = func]() mutable {
                        for (std::size_t start = i; start < stop; start++) {
                            func(data[start]);
                        }
                    })
                );
            }

            for (auto& res: chunk_results) {
                try {
                    res.get();
                } catch (const std::exception& e) {
                    if (!first_error) {
                        first_error = std::current_exception();
                    }
                }
            }

            if (first_error) {
                std::rethrow_exception(first_error);
            }
             
            
        }
    };

}