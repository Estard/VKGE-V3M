#include "../Definitions.hpp"
#include "../Components/VkSupportComponents.hpp"
#include "../Components/Vk3DComponents.hpp"
#include "InitialHelper.hpp"
#include "ImageHelper.hpp"

static PlayerEntity player;

class PresentationSystem
{
	public:
		GLFWwindow *window;
		VkSurfaceKHR surface;

		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
		VkRenderPass renderPass;

		VkDebugReportCallbackEXT debugReporter;

		vksc::Queues queues;
		vksc::Descriptors descriptors;
		vksc::SwapChain swapChain;
		vksc::Pipelines pipelines;
		vksc::Buffers buffers;
		vk3d::Image depthBuffer;
		VkFormat depthFormat;

		VkCommandPool cmdPool;
		VkCommandBuffer primaryCmdBuffer;

		vksc::Synchronizers synchronizers;

		PresentationSystem()
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			window = glfwCreateWindow(width, height, appName.c_str(),
					nullptr, nullptr);
			//Instance, Surface, Physical- und LogicalDevice, DepthFormat, DebugCallback, QueueFamilies
			createBase();

			//SwapChain,Image Views, RenderPass
			preparePresentation();

			//DescriptorSetLayout, PipelineLayout, GraphicsPipeline, CmdPool
			setupPipeline();

			//DepthImage, Framebuffer
			createInternalImages();

			//Semaphores, Fences
			createSynchronizers();

		}

		~PresentationSystem()
		{

			swapChainCleanup();
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vkDestroySemaphore(logicalDevice,
						synchronizers.imageAvailableSemas[i], nullptr);
				vkDestroySemaphore(logicalDevice,
						synchronizers.renderFinishedSemas[i], nullptr);
				vkDestroyFence(logicalDevice, synchronizers.inFlightFences[i],
						nullptr);
			}
			vkDestroyFence(logicalDevice, synchronizers.renderFence, nullptr);

			vkDestroyPipeline(logicalDevice, pipelines.solid, nullptr);
			vkDestroyPipelineLayout(logicalDevice, pipelines.layout, nullptr);

			vkDestroyDescriptorPool(logicalDevice, descriptors.dPool, nullptr);

			vkDestroyDescriptorSetLayout(logicalDevice, descriptors.sceneLayout,
					nullptr);
			vkDestroyDescriptorSetLayout(logicalDevice,
					descriptors.materialLayout, nullptr);

			vkDestroyBuffer(logicalDevice, buffers.uniformBuffer, nullptr);
			vkFreeMemory(logicalDevice, buffers.uniformBufferMemory, nullptr);
			if (buffers.vertexBuffer && buffers.indexBuffer)
			{
				vkDestroyBuffer(logicalDevice, buffers.vertexBuffer, nullptr);
				vkFreeMemory(logicalDevice, buffers.vertexBufferMemory,
						nullptr);
				vkDestroyBuffer(logicalDevice, buffers.indexBuffer, nullptr);
				vkFreeMemory(logicalDevice, buffers.indexBufferMemory, nullptr);
			}
			vkDestroyCommandPool(logicalDevice, cmdPool, nullptr);

			vkDestroyDevice(logicalDevice, nullptr);

			if (enableValidationLayers)
				DestroyDebugReportCallbackEXT(instance, debugReporter, nullptr);

			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);
			glfwDestroyWindow(window);
			glfwTerminate();
		}

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
				VkMemoryPropertyFlags properties, VkBuffer *buffer,
				VkDeviceMemory *bufferMemory)
		{
			VkBufferCreateInfo bufferInfo = vk::BufferCreateInfo( { }, size,
					vk::BufferUsageFlags(), vk::SharingMode::eExclusive);
			bufferInfo.usage = usage;

			vkCheck(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, buffer),
					"Buffer Erschaffung gescheitert");

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(logicalDevice, *buffer,
					&memRequirements);

			VkMemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo(
					memRequirements.size,
					findMemoryType(physicalDevice,
							memRequirements.memoryTypeBits, properties));

			vkCheck(
					vkAllocateMemory(logicalDevice, &allocInfo, nullptr,
							bufferMemory),
					"Buffer Memory Allocation gescheitert");

			vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);
		}

		void recreateSwapChain()
		{
			vkDeviceWaitIdle(logicalDevice);
			swapChainCleanup();
			preparePresentation();
			createInternalImages();
		}

	private:

		void createBase()
		{
			//Instance
			VkApplicationInfo appInfo = vk::ApplicationInfo(appName.c_str(), 1,
					engineName.c_str(), VK_MAKE_VERSION(0, 0, 2));

			auto glfwExtensions = aquireRequiredExtensions();

			VkInstanceCreateInfo instanceInfo = vk::InstanceCreateInfo();
			instanceInfo.pApplicationInfo = &appInfo;
			instanceInfo.enabledExtensionCount =
					static_cast<uint32_t>(glfwExtensions.size());
			instanceInfo.ppEnabledExtensionNames = glfwExtensions.data();
			instanceInfo.enabledLayerCount =
					enableValidationLayers ?
							static_cast<uint32_t>(validationLayers.size()) : 0;
			instanceInfo.ppEnabledLayerNames =
					enableValidationLayers ? validationLayers.data() : NULL;

			vkCheck(vkCreateInstance(&instanceInfo, nullptr, &instance),
					"Instance erschaffung gescheitert");
			//checkExtensionSupport();

			//DebugCallback
			if (enableValidationLayers)
			{
				VkDebugReportCallbackCreateInfoEXT debugCreateInfo =
						vk::DebugReportCallbackCreateInfoEXT();
				debugCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
						| VK_DEBUG_REPORT_WARNING_BIT_EXT;
				debugCreateInfo.pfnCallback = debugCallback;
				vkCheck(
						DebugReportCallbackEXT(instance, &debugCreateInfo,
								nullptr, &debugReporter),
						"Mein Debugger, NEIN!");
			}
			//Surface
			vkCheck(
					glfwCreateWindowSurface(instance, window, nullptr,
							&surface), "Kein Surface zum zeichnen");

			//Physical Device
			uint amountOfGPUs = 0;
			vkEnumeratePhysicalDevices(instance, &amountOfGPUs, nullptr);
			if (amountOfGPUs == 0)
				throw std::runtime_error(
						"Keine vulkanfähige Grafikkarte gefunden!");

			std::vector<VkPhysicalDevice> gpus(amountOfGPUs);
			vkEnumeratePhysicalDevices(instance, &amountOfGPUs, gpus.data());

			std::multimap<int, VkPhysicalDevice> candidates;
			for (const auto &gpu : gpus)
			{
				int score = rateGPU(gpu, surface);
				candidates.insert(std::make_pair(score, gpu));
			}

			if (candidates.rbegin()->first > 0)
				physicalDevice = candidates.rbegin()->second;
			else
				throw std::runtime_error("GPU nicht findbar");

			depthFormat = findDepthFormat(physicalDevice);

			//Logical Device
			queues.indices = findQueueFamilies(physicalDevice, surface);

			std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
			std::set<int> uniqueQueueFamilies = { queues.indices.graphicsFamily,
					queues.indices.presentFamily };

			float queuePriority = 1.0f;

			for (int queueFamily : uniqueQueueFamilies)
			{
				vk::DeviceQueueCreateInfo queueCreateInfo =
						vk::DeviceQueueCreateInfo( { }, queueFamily, 1,
								&queuePriority);
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures = { };
			deviceFeatures.samplerAnisotropy = VK_TRUE;

			VkDeviceCreateInfo deviceInfo = vk::DeviceCreateInfo(
					vk::DeviceCreateFlags(),
					static_cast<uint32_t>(queueCreateInfos.size()),
					queueCreateInfos.data(),
					enableValidationLayers ?
							static_cast<uint32_t>(validationLayers.size()) : 0,
					enableValidationLayers ? validationLayers.data() : nullptr);
			deviceInfo.pEnabledFeatures = &deviceFeatures;
			deviceInfo.enabledExtensionCount =
					static_cast<uint32_t>(deviceExtensions.size());
			deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

			vkCheck(
					vkCreateDevice(physicalDevice, &deviceInfo, nullptr,
							&logicalDevice),
					"Logischerweise ist das logische Device nicht erstellt worden");
			vkGetDeviceQueue(logicalDevice, queues.indices.graphicsFamily, 0,
					&queues.graphics);
			vkGetDeviceQueue(logicalDevice, queues.indices.presentFamily, 0,
					&queues.present);
		}

		void preparePresentation()
		{
			//SwapChain
			VkSwapchainCreateInfoKHR swapChainInfo = createSwapChainInfo(
					physicalDevice, surface);
			vkCheck(
					vkCreateSwapchainKHR(logicalDevice, &swapChainInfo, nullptr,
							&swapChain.swapChain),
					"SwapChain Konnte nicht erstellt werden");

			vkGetSwapchainImagesKHR(logicalDevice, swapChain.swapChain,
					&swapChainInfo.minImageCount, nullptr);
			swapChain.swapChainImages.resize(swapChainInfo.minImageCount);
			vkGetSwapchainImagesKHR(logicalDevice, swapChain.swapChain,
					&swapChainInfo.minImageCount,
					swapChain.swapChainImages.data());
			swapChain.swapChainImageFormat = swapChainInfo.imageFormat;
			swapChain.swapChainExtent = swapChainInfo.imageExtent;

			//Image Views
			swapChain.swapChainImageViews.resize(
					swapChain.swapChainImages.size());

			for (size_t i = 0; i < swapChain.swapChainImages.size(); i++)
			{
				swapChain.swapChainImageViews[i] = createImageView(
						logicalDevice, swapChain.swapChainImages[i],
						swapChain.swapChainImageFormat,
						VK_IMAGE_ASPECT_COLOR_BIT, 1);
			}

			//RenderPass
			VkAttachmentDescription colorAttachment = vk::AttachmentDescription(
					vk::AttachmentDescriptionFlags(),
					static_cast<vk::Format>(swapChain.swapChainImageFormat),
					vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
					vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare,
					vk::AttachmentStoreOp::eStore, vk::ImageLayout::eUndefined,
					vk::ImageLayout::ePresentSrcKHR);

			VkAttachmentReference colorAttachmentRef = vk::AttachmentReference(
					0, vk::ImageLayout::eColorAttachmentOptimal);

			VkAttachmentDescription depthAttachment = vk::AttachmentDescription(
					vk::AttachmentDescriptionFlags(),
					static_cast<vk::Format>(depthFormat),
					vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
					vk::AttachmentStoreOp::eDontCare,
					vk::AttachmentLoadOp::eDontCare,
					vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined,
					vk::ImageLayout::eDepthStencilAttachmentOptimal);

			VkAttachmentReference depthAttachmentRef = vk::AttachmentReference(
					1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			VkSubpassDescription subpass = { };
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			std::array<VkAttachmentDescription, 2> attachments = {
					colorAttachment, depthAttachment };
			VkRenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo(
					{ }, static_cast<uint32_t>(attachments.size()));
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VkSubpassDependency dependency = vk::SubpassDependency(
					static_cast<uint32_t>(VK_SUBPASS_EXTERNAL), 0,
					vk::PipelineStageFlagBits::eColorAttachmentOutput,
					vk::PipelineStageFlagBits::eColorAttachmentOutput);
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
					| VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			vkCheck(
					vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr,
							&renderPass), "RenderPass Erzeugung gescheitert");
		}

		void setupPipeline()
		{
			//DescriptorSetLayouts
			std::vector<VkDescriptorSetLayoutBinding> bindings = { };
			VkDescriptorSetLayoutBinding uboLayoutBinding =
					vk::DescriptorSetLayoutBinding(0,
							vk::DescriptorType::eUniformBuffer, 1,
							vk::ShaderStageFlagBits::eVertex);

			bindings.push_back(uboLayoutBinding);

			VkDescriptorSetLayoutCreateInfo layoutInfoUBO =
					vk::DescriptorSetLayoutCreateInfo( { },
							static_cast<uint32_t>(bindings.size()));
			layoutInfoUBO.pBindings = bindings.data();

			vkCheck(
					vkCreateDescriptorSetLayout(logicalDevice, &layoutInfoUBO,
							nullptr, &descriptors.sceneLayout),
					"Descriptor Set UBO Layout konnte nicht erstellt werden");

			VkDescriptorSetLayoutBinding samplerLayoutBinding =
					vk::DescriptorSetLayoutBinding(1,
							vk::DescriptorType::eCombinedImageSampler, 1,
							vk::ShaderStageFlagBits::eFragment);
			bindings.clear();
			bindings.push_back(samplerLayoutBinding);

			VkDescriptorSetLayoutCreateInfo layoutInfoM = { };
			layoutInfoM.sType =
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfoM.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfoM.pBindings = bindings.data();

			vkCheck(
					vkCreateDescriptorSetLayout(logicalDevice, &layoutInfoM,
							nullptr, &descriptors.materialLayout),
					"Descriptor Set Layout konnte nicht erstellt werden");

			//PipelineLayout
			auto vertShaderCode = readFile("Assets/shaders/vert.spv");
			auto fragShaderCode = readFile("Assets/shaders/frag.spv");

			VkShaderModule vertShaderModule = createShaderModule(logicalDevice,
					vertShaderCode);
			VkShaderModule fragShaderModule = createShaderModule(logicalDevice,
					fragShaderCode);

			VkPipelineShaderStageCreateInfo vertShaderStageInfo =
					vk::PipelineShaderStageCreateInfo(
							vk::PipelineShaderStageCreateFlags(),
							vk::ShaderStageFlagBits::eVertex,
							static_cast<vk::ShaderModule>(vertShaderModule),
							"main");

			VkPipelineShaderStageCreateInfo fragShaderStageInfo =
					vk::PipelineShaderStageCreateInfo(
							vk::PipelineShaderStageCreateFlags(),
							vk::ShaderStageFlagBits::eFragment,
							static_cast<vk::ShaderModule>(fragShaderModule),
							"main");

			VkPipelineShaderStageCreateInfo shaderStages[] = {
					vertShaderStageInfo, fragShaderStageInfo };

			auto bindingDescription = getBindingDescription();
			auto attributeDescription = getAttributeDescription();

			VkPipelineVertexInputStateCreateInfo vertexInputInfo =
					vk::PipelineVertexInputStateCreateInfo(
							vk::PipelineVertexInputStateCreateFlags(), 1, { },
							static_cast<uint32_t>(attributeDescription.size()));
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions =
					attributeDescription.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = { };
			inputAssembly.sType =
					VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			vk::Viewport viewport = vk::Viewport(0.f, 0.f,
					(float) swapChain.swapChainExtent.width,
					(float) swapChain.swapChainExtent.height, 0.f, 1.f);

			vk::Rect2D scissor = vk::Rect2D( { 0, 0 },
					static_cast<vk::Extent2D>(swapChain.swapChainExtent));

			VkPipelineViewportStateCreateInfo viewportState =
					vk::PipelineViewportStateCreateInfo(
							vk::PipelineViewportStateCreateFlags(), 1,
							&viewport, 1, &scissor);

			VkPipelineRasterizationStateCreateInfo rasterizer =
					vk::PipelineRasterizationStateCreateInfo(
							vk::PipelineRasterizationStateCreateFlags(),
							static_cast<vk::Bool32>(VK_FALSE),
							static_cast<vk::Bool32>(VK_FALSE),
							vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
							vk::FrontFace::eCounterClockwise,
							static_cast<vk::Bool32>(VK_FALSE), 0.f, 0.f, 0.f,
							1.f);

			VkPipelineMultisampleStateCreateInfo multisampling =
					vk::PipelineMultisampleStateCreateInfo(
							vk::PipelineMultisampleStateCreateFlags(),
							vk::SampleCountFlagBits::e1);
			multisampling.minSampleShading = 1.0f;

			VkPipelineDepthStencilStateCreateInfo depthStencil =
					vk::PipelineDepthStencilStateCreateInfo(
							vk::PipelineDepthStencilStateCreateFlags(),
							static_cast<vk::Bool32>(VK_TRUE),
							static_cast<vk::Bool32>(VK_TRUE),
							vk::CompareOp::eLess);
			depthStencil.maxDepthBounds = 1.0f;

			VkPipelineColorBlendAttachmentState colorBlendAttachment =
					vk::PipelineColorBlendAttachmentState(
							static_cast<vk::Bool32>(VK_FALSE),
							vk::BlendFactor::eOne, vk::BlendFactor::eZero,
							vk::BlendOp::eAdd, vk::BlendFactor::eOne,
							vk::BlendFactor::eZero, vk::BlendOp::eAdd);
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
					| VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
					| VK_COLOR_COMPONENT_A_BIT;


			VkPipelineColorBlendStateCreateInfo colorBlending =
					vk::PipelineColorBlendStateCreateInfo();
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;

			VkPushConstantRange pushConstRange = vk::PushConstantRange(
					vk::ShaderStageFlagBits::eVertex, 0,
					sizeof(vk3d::PushBlock));

			VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo =
					vk::PipelineDynamicStateCreateInfo();
			dynamicStateCreateInfo.pDynamicStates = dynamicStates;
			dynamicStateCreateInfo.dynamicStateCount = 2;

			std::array<VkDescriptorSetLayout, 2> layouts = {
					descriptors.sceneLayout, descriptors.materialLayout };

			VkPipelineLayoutCreateInfo pipelineLayoutInfo =
					vk::PipelineLayoutCreateInfo();
			pipelineLayoutInfo.setLayoutCount =
					static_cast<uint32_t>(layouts.size());
			pipelineLayoutInfo.pSetLayouts = layouts.data();
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

			vkCheck(
					vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo,
							nullptr, &pipelines.layout),
					"PipelineLayout Erschaffung Fehlgeschlagen");

			//Pipeline
			VkGraphicsPipelineCreateInfo pipelineInfo =
					vk::GraphicsPipelineCreateInfo( { }, 2);
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
			pipelineInfo.layout = pipelines.layout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.basePipelineIndex = -1;

			vkCheck(
					vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1,
							&pipelineInfo, nullptr, &pipelines.solid),
					"Graphics Pipeline konnte nicht erstellt werden");

			vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
			vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);

			//CommandPool
			VkCommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
			poolInfo.queueFamilyIndex = queues.indices.graphicsFamily;
			vkCheck(
					vkCreateCommandPool(logicalDevice, &poolInfo, nullptr,
							&cmdPool),
					"CommandPool konnte nicht gefüllt werden");
		}

		void createInternalImages()
		{
			//DepthBuffer
			createImage(logicalDevice, physicalDevice,
					swapChain.swapChainExtent.width,
					swapChain.swapChainExtent.height, depthFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBuffer.image,
					&depthBuffer.memory, 1);
			depthBuffer.view = createImageView(logicalDevice, depthBuffer.image,
					depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
			transitionImageLayout(logicalDevice, cmdPool, queues.graphics,
					depthBuffer.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

			//FrameBuffer
			size_t size = swapChain.swapChainImageViews.size();
			swapChain.swapChainFramebuffers.resize(size);

			for (size_t i = 0; i < size; i++)
			{
				std::array<VkImageView, 2> attachments = {
						swapChain.swapChainImageViews[i], depthBuffer.view };
				VkFramebufferCreateInfo framebufferInfo =
						vk::FramebufferCreateInfo();
				framebufferInfo.renderPass = renderPass;
				framebufferInfo.attachmentCount =
						static_cast<uint32_t>(attachments.size());
				framebufferInfo.pAttachments = attachments.data();
				framebufferInfo.width = swapChain.swapChainExtent.width;
				framebufferInfo.height = swapChain.swapChainExtent.height;
				framebufferInfo.layers = 1;

				vkCheck(
						vkCreateFramebuffer(logicalDevice, &framebufferInfo,
								nullptr, &swapChain.swapChainFramebuffers[i]),
						"Framebuffer, hat nicht geklappt");
			}
		}

		void createSynchronizers()
		{
			synchronizers.imageAvailableSemas.resize(MAX_FRAMES_IN_FLIGHT);
			synchronizers.renderFinishedSemas.resize(MAX_FRAMES_IN_FLIGHT);
			synchronizers.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

			VkSemaphoreCreateInfo semaInfo = vk::SemaphoreCreateInfo();
			VkFenceCreateInfo fenceInfo = vk::FenceCreateInfo(
					vk::FenceCreateFlagBits::eSignaled);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				vkCheck(
						vkCreateSemaphore(logicalDevice, &semaInfo, nullptr,
								&synchronizers.imageAvailableSemas[i]),
						"Problem with a imageAvailbaleSema");
				vkCheck(
						vkCreateSemaphore(logicalDevice, &semaInfo, nullptr,
								&synchronizers.renderFinishedSemas[i]),
						"Problem with a renderFinishedSema");
				vkCheck(
						vkCreateFence(logicalDevice, &fenceInfo, nullptr,
								&synchronizers.inFlightFences[i]),
						"Problem with a Fence");
			}
			fenceInfo = vk::FenceCreateInfo();
			vkCheck(
					vkCreateFence(logicalDevice, &fenceInfo, nullptr,
							&synchronizers.renderFence),
					"Problem with a Fence");
		}

		void swapChainCleanup()
		{
			vkDestroyImageView(logicalDevice, depthBuffer.view, nullptr);
			vkDestroyImage(logicalDevice, depthBuffer.image, nullptr);
			vkFreeMemory(logicalDevice, depthBuffer.memory, nullptr);
			vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
			for (auto framebuffer : swapChain.swapChainFramebuffers)
			{
				vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
			}
			for (auto imageView : swapChain.swapChainImageViews)
			{
				destroyImageView(logicalDevice, imageView);
			}
			vkDestroySwapchainKHR(logicalDevice, swapChain.swapChain, nullptr);
		}

};
