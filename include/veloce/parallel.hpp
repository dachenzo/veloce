#pragma once
#include <concepts>
#include <cstddef>
#include <exception>
#include <span>
#include <type_traits>
#include <vector>
#include <future>
#include <veloce/thread_pool.hpp>
#include <veloce/chunker.hpp>


namespace veloce {

    template<typename Func, typename T>
    concept VoidCopyableFunctionOn = std::invocable<Func, T&> && std::same_as<std::invoke_result_t<Func, T&>, void> && std::copy_constructible<std::remove_reference_t<Func>>;

    
    class Parallel {
        ThreadPool& pool;

        public:
        Parallel(ThreadPool& pool): pool{pool} {}
        Parallel(const Parallel& pool) = delete;
        Parallel(Parallel&& pool) = delete;

        template<typename T, std::size_t Extent,  VoidCopyableFunctionOn<T> Func>
        void operator()(std::span<T, Extent> data, Func&& func) {
            auto chunks = veloce::make_chunks(data, pool);
            std::vector<std::future<std::invoke_result_t<Func, T&>>> chunk_results;
            chunk_results.reserve(chunks.size()); 
            
            std::exception_ptr first_error;

            try {
                for (const auto& chunk : chunks) {
                    chunk_results.push_back(
                        pool.submit([data, chunk, func = func]() mutable {
                            for (std::size_t index = chunk.start;
                                index < chunk.end;
                                ++index) {
                                func(data[index]);
                            }
                        })
                    );
                }
            } catch (...) {
                first_error = std::current_exception();
            }

            for (auto& res: chunk_results) {
                try {
                    res.get();
                } catch (...) {
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