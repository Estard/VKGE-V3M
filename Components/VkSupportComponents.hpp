#pragma once
#include "../Definitions.hpp"
#include "../Entities/Entities.hpp"

namespace vksc
{

	struct SwapChainSupportDetails
	{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
	};

	struct SwapChain
	{
			VkSwapchainKHR swapChain;
			std::vector<VkImage> swapChainImages;
			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;
			std::vector<VkImageView> swapChainImageViews;
			std::vector<VkFramebuffer> swapChainFramebuffers;
	};

	struct Buffer
	{
			VkBuffer handle;
			VkDeviceMemory memory;
	};

	struct Buffers
	{
			VkBuffer vertexBuffer = VK_NULL_HANDLE;
			VkDeviceMemory vertexBufferMemory;
			VkBuffer indexBuffer = VK_NULL_HANDLE;
			VkDeviceMemory indexBufferMemory;
			VkBuffer uniformBuffer = VK_NULL_HANDLE;
			VkDeviceMemory uniformBufferMemory;
	};

	struct Pipelines
	{
			VkPipelineLayout layout;
			VkPipeline solid;
			//VkPipeline blend;
	};

	struct QueueFamiliyIndices
	{
			int graphicsFamily = -1;
			int presentFamily = -1;

			bool isComplete()
			{
				return (graphicsFamily >= 0) && (presentFamily >= 0);
			}
	};

	struct Queues
	{
			VkQueue graphics;
			VkQueue present;
			vksc::QueueFamiliyIndices indices;
	};

	struct Descriptors
	{
			VkDescriptorPool dPool;
			VkDescriptorSetLayout sceneLayout;
			VkDescriptorSetLayout materialLayout;
			VkDescriptorSet sceneSet;

	};

	struct Synchronizers
	{
			std::vector<VkSemaphore> imageAvailableSemas;
			std::vector<VkSemaphore> renderFinishedSemas;
			std::vector<VkFence> inFlightFences;
			VkFence renderFence;
			size_t currentFrame = 0;
	};

	struct ThreadData
	{
			VkCommandPool cmdPool;
			std::vector<VkCommandBuffer> commandBuffers;
			std::vector<vk3d::Transform> pushBlocks;
			std::vector<SceneEntity> objects;
	};

	struct UniformBufferData
	{
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 MVP;
			//glm::vec3 player;
	};
}

VkResult DebugReportCallbackEXT(VkInstance instance,
		const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
		const VkAllocationCallbacks *pAllocator,
		VkDebugReportCallbackEXT *pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(
			instance, "vkCreateDebugReportCallbackEXT");

	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugReportCallbackEXT(VkInstance instance,
		VkDebugReportCallbackEXT callback,
		const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(
			instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
		func(instance, callback, pAllocator);
}

