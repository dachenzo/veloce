#pragma once
#include <cstddef>
#include <span>
#include <vector>
#include <future>
#include <veloce/definitions.hpp>
#include <veloce/concepts.hpp>
#include <veloce/chunker.hpp>
#include <veloce/thread_pool.hpp>
#include <exception>

namespace veloce {


    class TransformReduce {
        ThreadPool& pool;

        public:
        TransformReduce(ThreadPool& pool): pool{pool} {} 
        TransformReduce(TransformReduce& other) = delete;
        TransformReduce(TransformReduce&& other) = delete;


        template <
            typename T,
            std::size_t Extent,
            CopyableEndoFunctionOn<T> Transformer,
            CopyableBinaryAssociativeFunctionOn<T> Reducer,
            ReduceOrder O = ReduceOrder::LeftRight
        >
        T operator()(std::span<T, Extent> data, T base, Transformer&& transform, Reducer&& reduce) {

            auto chunks = make_chunks(data, pool);
            std::vector<std::future<T>> chunk_results; 
            chunk_results.reserve(chunks.size());

            std::exception_ptr first_error;

            try {
                for (auto chunk: chunks) {
                    chunk_results.push_back(
                        pool.submit(
                            [chunk, transform = transform, reduce = reduce, data] () {
                                auto result = transform(data[chunk.start]);
                                for (std::size_t i = chunk.start+1; i < chunk.end; i++) {
                                    result = apply_reduce_order<O>(
                                        reduce, result, transform(data[i]));
                                }   
                                
                                return result;
                            }
                        )
                    );
                }

            } catch (...) {
                first_error = std::current_exception();
            }

            base = transform(base);
            for (auto& res: chunk_results) {
                try {
                    T next =  res.get();
                    base = apply_reduce_order<O>(reduce, base, next);
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