#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <future>
#include <exception>
#include <veloce/thread_pool.hpp>
#include <veloce/concepts.hpp>
#include <veloce/chunker.hpp>
#include <veloce/definitions.hpp>

namespace veloce {

    

    /*
    This is only correct for associative functions.

    */
    class Reduce {
        veloce::ThreadPool& pool;

        public:
        Reduce(veloce::ThreadPool& pool): pool{pool} {}
        Reduce(Reduce& other) = delete;
        Reduce(Reduce&& other) = delete;

        template<typename T, std::size_t Extent, veloce::CopyableBinaryAssociativeFunctionOn<T> Func, ReduceOrder O = ReduceOrder::LeftRight>
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
                                    result = apply_reduce_order<O>(func, result, data[i]);
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
                    base = apply_reduce_order<O>(func, base, next);
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