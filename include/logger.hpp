#pragma once 
#include "style.hpp"
#include "timer.hpp"
#include <cpptrace/basic.hpp>
#include <cpptrace/utils.hpp>
#include <cstdlib>
#include <libassert/assert.hpp>
inline constexpr void LOG_STR(
    std::string const& title_style, 
    std::string_view title, 
    std::string const& body_style="", 
    std::string const& body=""
){
    std::println("{}{}[{:4.2f}]{}: {}{}{}",
                 title_style,
                 title,
                 timer::get_seconds(timer::since_epoch()),
                 style::reset(),
                 body_style,
                 body,
                 style::reset()
                 );
}
template <class... _Args>
inline constexpr void LOG_EXIT(int code) {
    auto sty = style::fg_green();
    if (code != EXIT_SUCCESS){
        sty = style::fg_red() + style::underline();
    }
    LOG_STR(
        sty,
        "EXIT  ",
        sty,
        std::format("Exiting with error code {}",code)
    );
    if (code!= EXIT_SUCCESS){
        cpptrace::generate_trace(1).print(); 
    }
    std::exit(code);
}

template <class... _Args>
inline constexpr void LOG_ERROR(std::format_string<_Args...> a_fmt, _Args&&... a_args) {
    LOG_STR(
        style::fg_br_red(),
        "ERROR "sv,
        style::fg_red(),
        std::format(a_fmt,std::forward<_Args>(a_args)...)
    );
}
template <class... _Args>
inline constexpr void LOG_FATAL(std::format_string<_Args...> a_fmt, _Args&&... a_args) {
    LOG_STR(
        style::fg_br_red(),
        "FATAL "sv,
        style::fg_red(),
        std::format(a_fmt,std::forward<_Args>(a_args)...)
    );
    LOG_EXIT(EXIT_FAILURE);
}
template <class... _Args>
inline constexpr void LOG_INFO(std::format_string<_Args...> a_fmt, _Args&&... a_args) {
    LOG_STR(
        style::fg_blue(),
        "INFO  "sv,
        style::fg_br_blue(),
        std::format(a_fmt,std::forward<_Args>(a_args)...)
    );
}
template <class... _Args>
inline constexpr void LOG_WARN(std::format_string<_Args...> a_fmt, _Args&&... a_args) {
    LOG_STR(
        style::fg_yellow(),
        "WARN  "sv,
        style::fg_br_yellow(),
        std::format(a_fmt,std::forward<_Args>(a_args)...)
    );
}
