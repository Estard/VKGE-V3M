#pragma once
#include "../Definitions.hpp"

void checkExtensionSupport()
{
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extCount,
			extensions.data());

	std::cout << "EXTENSIONS PLEASE:" << std::endl;

	for (const auto &extension : extensions)
	{
		std::cout << "\t" << extension.extensionName << "\n";
	}
	std::cout << std::endl;

}

bool checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

	for (const char *layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto &layerProps : layers)
		{
			if (strcmp(layerName, layerProps.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}

	return true;
}

std::vector<const char*> aquireRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions,
			glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location,
		int32_t code, const char *layerPrefix, const char *msg, void *userData)
{
	std::cerr << "validation layer: " << msg << std::endl;
	return VK_FALSE;
}

vksc::QueueFamiliyIndices findQueueFamilies(VkPhysicalDevice device,
		VkSurfaceKHR surface)
{
	vksc::QueueFamiliyIndices indices;

	uint32_t queueFamilieCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilieCount,
			nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilieCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilieCount,
			queueFamilies.data());

	int i = 0;

	for (const auto &queueFamily : queueFamilies)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
				&presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (queueFamily.queueCount > 0
				&& (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			indices.graphicsFamily = i;
		}

		if (indices.isComplete())
			break;

		i++;
	}

	return indices;
}

int rateGPU(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	vksc::QueueFamiliyIndices indices = findQueueFamilies(device, surface);

	if (!deviceFeatures.geometryShader || !indices.isComplete())
		return 0;

	return score;
}

vksc::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
		VkSurfaceKHR surface)
{
	vksc::SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
			&details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
			nullptr);
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
				details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
			&presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
				&presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	if (availableFormats.size() == 1
			&& availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return
		{	VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	}

	for (const auto &availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
				&& availableFormat.colorSpace
						== VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto &availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilites)
{
	if (capabilites.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilites.currentExtent;
	else
	{
		VkExtent2D actualExtend = { width, height };

		actualExtend.width = std::max(capabilites.minImageExtent.width,
				std::min(capabilites.maxImageExtent.width, actualExtend.width));

		actualExtend.height = std::max(capabilites.maxImageExtent.height,
				std::min(capabilites.maxImageExtent.height,
						actualExtend.height));

		return actualExtend;
	}
}

VkDescriptorSet createDescriptorSet(VkDevice device,
		VkDescriptorSetLayout dSetLayout, VkDescriptorPool dPool)
{
	VkDescriptorSet dSet;

	VkDescriptorSetLayout layouts[] = { dSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo(
			static_cast<vk::DescriptorPool>(dPool), 1);
	allocInfo.pSetLayouts = layouts;

	vkCheck(vkAllocateDescriptorSets(device, &allocInfo, &dSet),
			"Allociation of descriptorset failed");

	return dSet;
}

void createWriteDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet,
		VkBuffer uniformBuffer, VkImageView imageView, VkSampler sampler)
{

	std::vector<VkWriteDescriptorSet> descriptorWrites = { };

	VkWriteDescriptorSet writeSet = { };
	if (uniformBuffer == VK_NULL_HANDLE && sampler != VK_NULL_HANDLE)
	{
		VkDescriptorImageInfo imageInfo = vk::DescriptorImageInfo(vk::Sampler(),
				vk::ImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet = descriptorSet;
		writeSet.dstBinding = 1;
		writeSet.dstArrayElement = 0;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet.descriptorCount = 1;
		writeSet.pImageInfo = &imageInfo;
		writeSet.pBufferInfo = nullptr;

	}
	else if (uniformBuffer != VK_NULL_HANDLE && sampler == VK_NULL_HANDLE)
	{
		VkDescriptorBufferInfo bufferInfo = { };
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(vksc::UniformBufferData);

		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet = descriptorSet;
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = 0;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeSet.descriptorCount = 1;
		writeSet.pImageInfo = nullptr;
		writeSet.pBufferInfo = &bufferInfo;
	}

	descriptorWrites.push_back(writeSet);

	vkUpdateDescriptorSets(device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(), 0, nullptr);

}



