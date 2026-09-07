#pragma once
#include <ranges>
#include <exception>
#include <future>
#include <stdexcept>
#include <veloce/chunker.hpp>
#include <veloce/concepts.hpp>
#include <veloce/thread_pool.hpp>

namespace veloce {

    class Transform {
        veloce::ThreadPool& pool;
        
        public:
        Transform(veloce::ThreadPool& pool): pool{pool} {}
        Transform(Transform& other) = delete;
        Transform(Transform&& other) = delete;

        template<
            typename Func,
            typename Input,
            typename Output
        >
        requires veloce::RandomAccessReadWriteOn<Input, Output, Func>
        void operator()(Func&& func, Input&& in, Output&& out) {
            if (std::ranges::size(in) > std::ranges::size(out)) {
                throw std::runtime_error("Input Container size is less than output container");
            }
            auto chunks = veloce::make_chunks(in, pool);

            std::exception_ptr first_error;
            std::vector<std::future<void>> chunk_results; chunk_results.reserve(chunks.size());

            try {
                for (auto& chunk: chunks) {
                    chunk_results.push_back(
                            pool.submit([chunk, func = func, &in, &out]() mutable {
                            auto in_it = std::ranges::begin(in) + chunk.start;
                            auto out_it = std::ranges::begin(out) + chunk.start;
                            auto end = std::ranges::begin(in) + chunk.end;
                            for (; in_it < end; in_it++) {
                                *out_it = func(*in_it);
                                ++out_it;
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