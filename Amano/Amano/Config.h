#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef NDEBUG
constexpr bool cEnableValidationLayers = false;
#else
constexpr bool cEnableValidationLayers = true;
#endif