#pragma once

template <typename T> //
using no_cvref = std::remove_cvref<T>;

template <typename T, typename Fn, typename... Args> //
concept result_type_is = std::same_as<std::invoke_result_t<Fn, Args...>, T>;
