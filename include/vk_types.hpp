#pragma once

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <format>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "common_concepts.hpp"
#include "logger.hpp"
#include "vk_concepts.hpp"

inline constexpr void vk_check(VkResult err) noexcept {
    if (err != VK_SUCCESS) {
        LOG_FATAL("VULKAN ERROR: {}", string_VkResult(err));
    }
}
inline constexpr void vk_check(vk::Result err) noexcept {
    return vk_check(static_cast<VkResult>(err));
}
template <typename From> //
inline constexpr auto get_c_handle(From const& val) noexcept {
    if constexpr (vkhpp_handle<From>) {
        using vkhpp_type = From;
        return static_cast<vkhpp_type::CType>(val);
    } else if constexpr (vkhpp_raii<From>) {
        using vkhpp_type = From::CppType;
        return static_cast<vkhpp_type::CType>(*val);
    } else {
        static_assert(false, "T is not a vkhpp type");
    }
}

