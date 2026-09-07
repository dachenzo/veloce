#pragma once
#include "veloce/concepts.hpp"
#include <span>
#include <ranges>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <veloce/thread_pool.hpp>

namespace veloce {
    struct SpanChunk {
        std::size_t start;
        std::size_t end;
    };

    template<typename T, std::size_t Extent>
    std::vector<SpanChunk> make_chunks(std::span<T, Extent> data, ThreadPool& pool) {
        auto n = data.size();
        auto worker_cnt =std::min(n,  pool.worker_cnt());
        
        if (n == 0) {
            return {};
        }

        if (worker_cnt == 0) {
            throw std::runtime_error("Zero workers found in pool");
        }

        auto step = n / worker_cnt;
        auto extra = n % worker_cnt;

        std::vector<SpanChunk> chunks; chunks.reserve(worker_cnt);
        auto increment = [&extra, &step](std::size_t i, bool mut) {
            if (extra > 0) {
                extra-= mut ? 1 : 0;
                return i+1+step;
            }
            return i+step;
        };

        for (std::size_t i = 0; i < n; i =  increment(i, true)) {
            auto stop = std::min(n, increment(i, false));
            chunks.emplace_back(i, stop);
        }

        return chunks;
    }


    template <RandomAccessSizedRange Input>
    std::vector<SpanChunk> make_chunks(Input data, ThreadPool &pool) {
        auto n = std::ranges::size(data);
        auto worker_cnt =std::min(n,  pool.worker_cnt());
        
        if (n == 0) {
            return {};
        }

        if (worker_cnt == 0) {
            throw std::runtime_error("Zero workers found in pool");
        }

        auto step = n / worker_cnt;
        auto extra = n % worker_cnt;

        std::vector<SpanChunk> chunks; chunks.reserve(worker_cnt);
        auto increment = [&extra, &step](std::size_t i, bool mut) {
            if (extra > 0) {
                extra-= mut ? 1 : 0;
                return i+1+step;
            }
            return i+step;
        };

        for (std::size_t i = 0; i < n; i = increment(i, true)) {
            auto stop = std::min(n, increment(i, false));
            chunks.emplace_back(i, stop);
        }

        return chunks;
    }
}