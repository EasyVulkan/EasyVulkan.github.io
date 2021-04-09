#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#define  GLM_FORCE_DEPTH_ZERO_TO_ONE
#define  GLM_FORCE_LEFT_HANDED
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <vulkan/vulkan.h>
#pragma comment(lib, "vulkan-1.lib")

#define EASY_VULKAN_BEGIN namespace easyVulkan {
#define NAMESPACE_END }

#define DISABLE_VK_LAYERS_AND_EXTENSIONS_CHECK