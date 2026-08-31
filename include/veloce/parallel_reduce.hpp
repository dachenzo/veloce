#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <future>
#include <exception>
#include <veloce/thread_pool.hpp>
#include <veloce/concepts.hpp>
#include <veloce/chunker.hpp>

namespace veloce {

    enum class ReduceOrder {
        LeftRight,
        RightLeft
    };

    /*
    This is only correct for associative functions.

    */
    class ParallelReduce {
        veloce::ThreadPool& pool;

        public:
        ParallelReduce(veloce::ThreadPool& pool): pool{pool} {}
        ParallelReduce(ParallelReduce& other) = delete;
        ParallelReduce(ParallelReduce&& other) = delete;

        template<typename T, std::size_t Extent, veloce::BinaryCopyableAssociativeFunctionOn<T> Func, ReduceOrder O>
        T operator()(std::span<T, Extent> data, T base, Func&& func) {
            auto chunks = veloce::make_chunks(data, pool);

            std::vector<std::future<T>> chunk_results; chunk_results.reserve(chunks.size());
            std::exception_ptr first_error;

            try {
                for (auto chunk: chunks) {
                    chunk_results.push_back(
                        pool.submit(
                        [chunk, func = func, data] () mutable {
                                auto result = data[chunk.start];
                                for (std::size_t i = chunk.start+1; i < chunk.end; i++) {
                                    if constexpr (O == ReduceOrder::LeftRight) {
                                        result = func(result, data[i]);
                                    } else {
                                        result = func(data[i], result);
                                    }
                                }

                                return result;
                            }
                        )
                    );
                }
            } catch (...) {
                first_error = std::current_exception();
            }

            for (auto& res: chunk_results) {

                try {
                    T next =  res.get();
                    if (O == ReduceOrder::LeftRight) {
                        base = func(base, next);
                    } else {
                        base = func(next, base);
                    }
                } catch (...) {
                    if (!first_error) {
                        first_error = std::current_exception();
                    }
                }
            }

            if (first_error) {
                std::rethrow_exception(first_error);
            }

            return base;

        }
    };
}