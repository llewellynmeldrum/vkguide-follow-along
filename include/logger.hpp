#pragma once 
template <class... _Args>
inline constexpr void LOG_ERROR(std::format_string<_Args...> a_fmt, _Args&&... a_args) {
    std::println(stderr, a_fmt, std::forward<_Args>(a_args)...);
}
template <class... _Args>
inline constexpr void LOG_EXIT(int e) {
    std::println(stderr, "Exiting with error code: {}.", e);
    std::exit(1);
}
