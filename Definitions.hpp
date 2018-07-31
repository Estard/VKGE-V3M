#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include "vulkan/vulkan.h"
#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>
#include <array>
#include <unordered_map>
#include <list>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "external/stb_image.h"
#include "external/tiny_obj_loader.h"
#include "external/Threads.hpp"

#include "Assets/generators/SimplexNoise.hpp"

enum SystemFlag
	: uint64_t
	{
		VGE_SYSTEM_STANDARD = 0, VGE_SYSTEM_RENDERER = 1 << 0, VGE_SYSTEM_AI = 1
				<< 1,
				VGE_SYSTEM_ENEMY = 1 << 2,
				VGE_SYSTEM_TURRET = 1 << 3,
				VGE_SYSTEM_PHYSIK = 1 << 4,
				VGE_SYSTEM_CANNON = 1 << 5
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

const unsigned int threadCount = thread::hardware_concurrency();

const uint32_t width = 1280;
const uint32_t height = 720;

const std::string engineName = "VKGE-V3M";
const std::string appName = "VKGE-V3M";

const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation" };

const std::vector<const char*> deviceExtensions = {
VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const float pi = 3.14159265358979;

static std::chrono::_V2::system_clock::time_point startTime =
		std::chrono::high_resolution_clock::now();

static float deltaTime = 0;

static std::vector<char> readFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Konnte File nicht öffnen");

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

static void vkCheck(VkResult result, std::string message)
{
	if (result != VK_SUCCESS)
		throw std::runtime_error(message);
}

