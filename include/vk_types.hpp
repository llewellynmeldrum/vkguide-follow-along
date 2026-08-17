#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>


#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

#include <format>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

inline constexpr void vk_check(vk::Result err) noexcept{
    if (err != vk::Result::eSuccess){
        std::println("VULKAN ERROR: {}", string_VkResult(static_cast<VkResult>(err)));
    }
}
template<typename From>
inline constexpr auto to_ctype(From&& val) noexcept{
    using fromType = std::remove_cvref_t<From>(decltype(val));
    using resType = fromType::CType;
    return static_cast<resType>(val);

}
