#pragma once
#include "../Definitions.hpp"

VkCommandBuffer beginSingleTimeCommands(VkDevice device,
		VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool,
		VkQueue graphicsQueue, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = vk::SubmitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue); //Alternativ vkWaitForFences

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkImageView createImageView(VkDevice logicalDevice, VkImage image,
		VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageSubresourceRange subresourceRange = vk::ImageSubresourceRange(
			vk::ImageAspectFlags(aspectFlags), 0, mipLevels, 0, 1);

	VkImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo();
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = subresourceRange;

	VkImageView imageView = { };
	vkCheck(vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView),
			"Ein Image View konnte nicht erstellt werden");
	return imageView;
}

inline void destroyImageView(VkDevice logicalDevice, VkImageView imageView)
{
	vkDestroyImageView(logicalDevice, imageView, nullptr);
}
VkSwapchainCreateInfoKHR createSwapChainInfo(VkPhysicalDevice physicalDevice,
		VkSurfaceKHR surface)
{
	vksc::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(
			physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
			swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(
			swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.maxImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0
			&& imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vksc::QueueFamiliyIndices indices = findQueueFamilies(physicalDevice,
			surface);
	uint32_t queueFamilieIndices[] = { (uint32_t) indices.graphicsFamily,
			(uint32_t) indices.presentFamily };

	VkSwapchainCreateInfoKHR swapChainInfo = vk::SwapchainCreateInfoKHR();
	swapChainInfo.surface = surface;
	swapChainInfo.minImageCount = imageCount;
	swapChainInfo.imageFormat = surfaceFormat.format;
	swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainInfo.imageExtent = extent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode = presentMode;
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;
	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilieIndices;
	}
	else
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainInfo.queueFamilyIndexCount = 0;
		swapChainInfo.pQueueFamilyIndices = nullptr;
	}
	return swapChainInfo;
}

VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
		VkImageTiling tiling, VkFormatFeatureFlags features,
		VkPhysicalDevice physicalDevice)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR
				&& (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL
				&& (props.optimalTilingFeatures & features) == features)
			return format;

	}
	throw std::runtime_error("Keine unterstützden Formate gefunden");

	return candidates[0];
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
{
	return findSupportedFormat( { VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, physicalDevice);
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> code)
{
	VkShaderModuleCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)
			!= VK_SUCCESS)
		throw std::runtime_error("Shader Modul konnte nicht erstellt werden");

	return shaderModule;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
		VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i))
				&& (memProperties.memoryTypes[i].propertyFlags & properties)
						== properties)
			return i;
	}

	throw std::runtime_error("Kein passender Speichertyp gefunden");
}

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
		VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties, VkBuffer *buffer,
		VkDeviceMemory *bufferMemory)
{
	VkBufferCreateInfo bufferInfo = vk::BufferCreateInfo( { }, size,
			static_cast<vk::BufferUsageFlagBits>(usage),
			vk::SharingMode::eExclusive);

	vkCheck(vkCreateBuffer(device, &bufferInfo, nullptr, buffer),
			"Vertex Buffer Erschaffung gescheitert");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo(
			memRequirements.size,
			findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
					properties));

	vkCheck(vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory),
			"Buffer Memory Allocation gescheitert");

	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

void createImage(VkDevice device, VkPhysicalDevice physicalDevice,
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage *image, VkDeviceMemory *imageMemory, uint32_t mipLevels)
{

	VkImageCreateInfo imageInfo = vk::ImageCreateInfo( { }, vk::ImageType::e2D,
			static_cast<vk::Format>(format), vk::Extent3D(width, height, 1),
			mipLevels, 1);
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;

	vkCheck(vkCreateImage(device, &imageInfo, nullptr, image),
			"Bilderschaffung nicht erfolgreich");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, *image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo(
			memRequirements.size,
			findMemoryType(physicalDevice, memRequirements.memoryTypeBits,
					properties));

	vkCheck(vkAllocateMemory(device, &allocInfo, nullptr, imageMemory),
			"Allocation of memory for image failed");

	vkBindImageMemory(device, *image, *imageMemory, 0);
}
bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT
			|| format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(VkDevice device, VkCommandPool commandPool,
		VkQueue graphicsQueue, VkImage image, VkFormat format,
		VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device,
			commandPool);

	VkImageMemoryBarrier barrier = vk::ImageMemoryBarrier();
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage, destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
			&& newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			&& newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
			&& newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
				| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}

	else
		throw std::runtime_error("unsupported layout transition");
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
			nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

void copyBufferToImage(VkDevice device, VkCommandPool cmdPool, VkQueue gQueue,
		VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, cmdPool);

	VkBufferImageCopy region = vk::BufferImageCopy(0, 0, 0,
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0,
					1), vk::Offset3D(0, 0, 0), vk::Extent3D(width, height, 1));

	vkCmdCopyBufferToImage(commandBuffer, buffer, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(device, cmdPool, gQueue, commandBuffer);
}

VkSampler createTextureSampler(VkDevice device, uint32_t mipLevels)
{
	VkSampler textureSampler;
	VkSamplerCreateInfo samplerInfo = vk::SamplerCreateInfo( { },
			vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0.f, static_cast<vk::Bool32>(VK_TRUE), 16);
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	vkCheck(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler),
			"Texture Sampler konnte nicht erstellt werden");

	return textureSampler;
}

void generateMipmaps(VkDevice device, VkCommandPool cmdPool, VkQueue gQueue,
		VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	VkCommandBuffer cmdBuffer = beginSingleTimeCommands(device, cmdPool);

	VkImageMemoryBarrier barrier = vk::ImageMemoryBarrier();
	barrier.image = image;
	barrier.subresourceRange = vk::ImageSubresourceRange(
			vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&barrier);

		VkImageBlit blit = vk::ImageBlit(
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor,
						i - 1, 0, 1),
				{ vk::Offset3D(0, 0, 0), vk::Offset3D(mipWidth, mipHeight, 1) },
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i,
						0, 1));
		blit.dstOffsets[0] =
		{	0,0,0};
		blit.dstOffsets[1] =
		{	mipWidth/2,mipHeight/2,1};

		vkCmdBlitImage(cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
				VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
				nullptr, 1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
			&barrier);

	endSingleTimeCommands(device, cmdPool, gQueue, cmdBuffer);

}
vk3d::Texture createTexture(VkDevice device, VkPhysicalDevice physicalDevice,
		VkCommandPool cmdPool, VkQueue gQueue, std::string filename)
{
	vk3d::Texture tex;
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(("Assets/textures/" + filename).c_str(),
			&texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(
			std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels)
		throw std::runtime_error("Texture konnte nicht geladen werden");

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(device, physicalDevice, imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
					| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
			&stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	createImage(device, physicalDevice, texWidth, texHeight,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
					| VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.memory,
			mipLevels);

	transitionImageLayout(device, cmdPool, gQueue, tex.image,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	copyBufferToImage(device, cmdPool, gQueue, stagingBuffer, tex.image,
			static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	generateMipmaps(device, cmdPool, gQueue, tex.image, texWidth, texHeight,
			mipLevels);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	tex.view = createImageView(device, tex.image, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	tex.sampler = createTextureSampler(device, mipLevels);

	return tex;
}

void copyBuffer(VkDevice device, VkCommandPool cmdPool, VkQueue graphicsQueue,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, cmdPool);

	VkBufferCopy copyRegion = { };
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(device, cmdPool, graphicsQueue, commandBuffer);
}
