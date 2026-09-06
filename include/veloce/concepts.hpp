#pragma once
#include <concepts>
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

    template<typename Func, typename T>
    concept CopyableEndoFunctionOn = std::invocable<Func, T> &&
    std::same_as<std::invoke_result_t<Func, T>, T> &&  CopyableFunction<Func>;
}