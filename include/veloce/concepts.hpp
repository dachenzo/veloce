#include <concepts>
#include <type_traits>

namespace veloce {

    template<typename Func, typename T>
    concept VoidCopyableFunctionOn = std::invocable<Func, T&> && std::same_as<std::invoke_result_t<Func, T&>, void> && std::copy_constructible<std::remove_reference_t<Func>>;


    template<typename  Func, typename T>
    concept BinaryCopyableAssociativeFunctionOn = std::invocable<Func, T, T> && std::same_as<std::invoke_result_t<Func, T, T>, T> && std::copy_constructible<std::remove_reference_t<Func>>;
}