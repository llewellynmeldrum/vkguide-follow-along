#pragma once

// Matches resource-managing Vulkan RAII wrappers (e.g.,
template <typename T>
concept vkhpp_raii = requires(T t){
    typename T::CType; // Trivial handles define their raw C-type
    typename T::CppType; // Trivial handles define their raw C-type
    { *t } -> std::convertible_to<typename T::CppType>;
};

// Matches vulkan hpp handle types
template <typename T>
concept vkhpp_handle = !vkhpp_raii<T> &&  std::copyable<T> && requires(T t) {
    typename T::CType; // Trivial handles define their raw C-type
    { static_cast<typename T::CType>(t) }; // Casts directly to C handle
};
