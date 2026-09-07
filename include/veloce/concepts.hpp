#pragma once
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace veloce {

    template<typename Func>
    concept CopyableFunction = std::copy_constructible<std::remove_reference_t<Func>>;

    template<typename Func, typename T>
    concept VoidCopyableFunctionOn = std::invocable<Func, T&> && std::same_as<std::invoke_result_t<Func, T&>, void> && CopyableFunction<Func>;

    template <typename Func, typename T> 
    concept BinaryAssociativeFunctionOn = std::invocable<Func, T, T> && std::same_as<std::invoke_result_t<Func, T, T>, T>;

    template<typename  Func, typename T>
    concept CopyableBinaryAssociativeFunctionOn = BinaryAssociativeFunctionOn<Func, T> &&  CopyableFunction<Func>;

    template<typename Func, typename T, typename R> 
    concept CopyableUnaryFunction = std::invocable<Func, T> && std::same_as<std::invoke_result_t<Func, T>, R> && CopyableFunction<Func>;

    template<typename Func, typename T>
    concept CopyableEndoFunctionOn = CopyableUnaryFunction<Func, T, T>;

    template <
        typename Input,
        typename Output,
        typename Func
    >
    concept RandomAccessReadWriteOn =
        std::ranges::random_access_range<Input> &&
        std::ranges::sized_range<Input> &&
        std::ranges::random_access_range<Output> &&
        std::ranges::sized_range<Output> &&
        CopyableFunction<Func> &&
        std::invocable<Func&, std::ranges::range_reference_t<Input>> &&
        std::indirectly_writable<std::ranges::iterator_t<Output>, 
        std::invoke_result_t<Func&,  std::ranges::range_reference_t<Input>>>;
}