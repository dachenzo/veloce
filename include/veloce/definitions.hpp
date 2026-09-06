#pragma once
#include <veloce/concepts.hpp>

namespace veloce {
    enum class ReduceOrder {
        LeftRight,
        RightLeft
    };

    template<ReduceOrder O = ReduceOrder::LeftRight, typename T, BinaryAssociativeFunctionOn<T> Reducer>
    constexpr T apply_reduce_order(Reducer&& reducer, T first, T second) {
        if constexpr (O == ReduceOrder::LeftRight) {
            return reducer(first, second);
        } else {
            return reducer(second, first);
        }
    }
}