#include "vulkanTools/vulkanInstance.h"


namespace TDS
{
	VulkanInstance::VulkanInstance(const WindowsWin& _Windows)
	{
		VkResult err;

		// Vulkan instance
		err = createInstance(_Windows.settings.validation);
		if (err) {
			std::cerr << "Could not create Vulkan instance :\n";
		}
		
		//if request, enable default validation layers for debugging
		if (_Windows.settings.validation)
		{
			enableValidate = true; //to be remove
			TDS::Debug::setupDebugger(m_VKhandler);
		}

		//win32 surface creation

		{
			auto pFNVKCreateWin32Surface
			{
			  reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>
			  (
				vkGetInstanceProcAddr(m_VKhandler, "vkCreateWin32SurfaceKHR")
			  )
			};
			if (nullptr == pFNVKCreateWin32Surface)
			{
				std::cerr << "Vulkan Driver missing the VK_KHR_win32_surface extension\n";
			}

			VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
			surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surfaceCreateInfo.pNext = nullptr;
			surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
			surfaceCreateInfo.hwnd = _Windows.getWindowHandler();
			err = vkCreateWin32SurfaceKHR(m_VKhandler, &surfaceCreateInfo, nullptr, &m_Surface);

			if(err != VK_SUCCESS)
				throw std::runtime_error("failed to create window surface!");
			
		}



		//get physical device aka gpu card
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(m_VKhandler, &deviceCount, nullptr);

			if (deviceCount == 0) // if there are 0 devices with vulkan support, ggwp
			{
				throw std::runtime_error("failed to find GPUs with Vulkan support!");
			}

			//otherwise, allocate an array to hold all of the VkPhysicalDevice handles
			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(m_VKhandler, &deviceCount, devices.data());

			//evaluate each of them and check suitability
			for (const auto& device : devices) {
				if (isDeviceSuitable(device)) {
					m_PhysDeviceHandle = device;
					break;
				}
			}

			if (m_PhysDeviceHandle == VK_NULL_HANDLE) {
				throw std::runtime_error("failed to find a suitable GPU!");
			}
		}
		
		//create logical devices
		{
			QueueFamilyIndices indices = findQueueFamilies(m_PhysDeviceHandle);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> uniqueQueueFamilies = 
			{ 
			  indices.graphicsFamily.value(), 
			  indices.presentFamily.value() 
			};

			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies) 
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount		 = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}
			VkPhysicalDeviceFeatures deviceFeatures{};

			VkDeviceCreateInfo createInfo{};
			  createInfo.sType				     = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			  createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
			  createInfo.pQueueCreateInfos	     = queueCreateInfos.data();
			  createInfo.pEnabledFeatures		 = &deviceFeatures;
			  createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
			  createInfo.ppEnabledExtensionNames = deviceExtensions.data();
			
			if (_Windows.settings.validation)
			{
			  createInfo.enabledLayerCount       = static_cast<uint32_t>(validationLayers.size());
			  createInfo.ppEnabledLayerNames     = validationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			if (vkCreateDevice(m_PhysDeviceHandle, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create logical device!");
			}

			vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicQueue);
			vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value() , 0, &m_presentQueue);
		}
		//swapchain setup
		{
			createSwapChain(_Windows);
		}
		//image View
		{
			createImageViews();
		}
		//render pass
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = m_swapChainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			if (vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}

		}
		
		{
			createDescriptorSetLayout();
		}

		//graphics pipeline???
		{
			auto vertShaderCode = readFile("../assets/shaders/vert.spv");
			auto fragShaderCode = readFile("../assets/shaders/frag.spv");

			VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
			VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

			VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";

			shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

			
			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			//get the 2 descriptions from struct vertex
			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDescription = Vertex::getAttributeDescriptions();
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
			
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer{};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //carefull with face
			rasterizer.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampling{};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;


			//Dynamic state make it more flexible to feed in data in draw time?
			std::vector<VkDynamicState> dynamicStates
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.pNext = VK_NULL_HANDLE;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();
			//dynamicstate.pFlages ??? dont need?

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 0;

			if (vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages.data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = m_pipelineLayout;
			pipelineInfo.renderPass = m_RenderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			if (vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicPipeline) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}



			vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr);
			vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr);

		}

		//framebuffers
		{
			createFrameBuffer();
		}




		//command pool
		{
			QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysDeviceHandle);

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

			if (vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create command pool!");
			}
		}
		//vertexBuffer
		{
			createVertexBuffer();
		}
		//Indexbuffer
		{
			createIndexBuffer();
		}
		//create UniformBuffer();
		{
			createUniformBuffers();
		}

		//create descriptorPool
		{
			createDescriptorPool();
			createDescriptorSets();
		}


		//command buffer
		{
			m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = m_commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

			if (vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}
		}
		//create sync object
		{
			//all need to resize as per frame needed
			m_imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
			m_renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
			m_inFlightFence.resize(MAX_FRAMES_IN_FLIGHT);


			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (size_t i{0}; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore[i]) != VK_SUCCESS ||
					vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore[i]) != VK_SUCCESS ||
					vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFence[i]) != VK_SUCCESS) 
					{
						throw std::runtime_error("failed to create synchronization objects for a frame!");
					}
			}
		}

	}

	VulkanInstance::~VulkanInstance()
	{
		cleanupSwapChain();

		vkDestroyPipeline(m_logicalDevice, m_graphicPipeline, nullptr);
		vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_logicalDevice, m_RenderPass, nullptr);

		for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(m_logicalDevice, uniformBuffers[i], nullptr);
			vkFreeMemory(m_logicalDevice,uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(m_logicalDevice,m_descriptorSetLayout, nullptr);

		vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
		vkFreeMemory(m_logicalDevice, m_IndexBufferMemory, nullptr);

		vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
		vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);



		for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore[i], nullptr);
			vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore[i], nullptr);
			vkDestroyFence(m_logicalDevice, m_inFlightFence[i], nullptr);
		}

		vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);// command pool will free command buffers for us.

		vkDestroyDevice(m_logicalDevice, nullptr);

		if(enableValidate)
			TDS::Debug::freeDebugger(m_VKhandler);

		vkDestroySurfaceKHR(m_VKhandler, m_Surface, nullptr);
		vkDestroyInstance(m_VKhandler, nullptr);
	}

	VkResult VulkanInstance::createInstance(bool enableValidation)
	{
		//feed our application info to struct VkApplicationInfo
		VkApplicationInfo appInfo{};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		//appInfo.pNext				= nullptr;
		appInfo.pApplicationName	= "Tear Drop Studio";
		//appInfo.applicationVersion	= 1;
		appInfo.pEngineName			= "Tear Drop Engine";
		//appInfo.engineVersion		= 1;
		appInfo.apiVersion			= apiVersion;


		//enabling Extensions
		std::vector<const char*> instanceExtensions
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
		};

		// Get extensions supported by the instance and store for later use
		uint32_t extCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (const VkExtensionProperties& extension : extensions)
				{
					supportedInstanceExtensions.push_back(extension.extensionName);
				}
			}
		}

		// Enabled requested instance extensions
		if ( enabledInstanceExtensions.size() > 0)
		{
			for (const char* enabledExtension : enabledInstanceExtensions)
			{
				// Output message if requested extension is not available
				if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
				{
					std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
				}
				instanceExtensions.push_back(enabledExtension);
			}
		}


		//Instance create info
		VkInstanceCreateInfo instanceCreateInfo{};
		{
			instanceCreateInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCreateInfo.pNext				= nullptr;
			//instanceCreateInfo.flags				= 0;
			instanceCreateInfo.pApplicationInfo		= &appInfo;

			//Validation Layering
			if (enableValidation)
			{
				if (checkValidationLayerSupport())
				{
					instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
				}
				else
				{
					std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled\n";
				}
			}
			
			if (enableValidation || std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
				instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			if (instanceExtensions.size() > 0)
			{
				instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
				instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
			}	
		};



		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_VKhandler);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
		// If the debug utils extension is present we set up debug functions, so samples an label objects for debugging
		if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) 
		{
			//to do 
			std::cout << "ok\n";
		}

		return result;

	}


	//checks if all of the requested layers are available
	bool VulkanInstance::checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}


		return true;
	}
	VulkanInstance::SwapChainSupportDetails VulkanInstance::querySwapChainSupport(const VkPhysicalDevice& device)
	{
		SwapChainSupportDetails details;

		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities) != VK_SUCCESS)
			std::cerr << "error";

		uint32_t formatCount;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr) != VK_SUCCESS)
			std::cerr << "error";

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data()) != VK_SUCCESS)
				std::cerr << "error";
		}

		uint32_t presentModeCount;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr) != VK_SUCCESS)
			std::cerr << "error";

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			if(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data()) != VK_SUCCESS)
				std::cerr << "error";
		}

		return details;
	}
	bool VulkanInstance::isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;

		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	VulkanInstance::QueueFamilyIndices VulkanInstance::findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i{ 0 };
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	bool VulkanInstance::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	VkSurfaceFormatKHR VulkanInstance::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}
	
	VkPresentModeKHR VulkanInstance::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}
	
	VkExtent2D VulkanInstance::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const WindowsWin &windows)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			

			VkExtent2D actualExtent = {
				windows.getWidth(),
				windows.getHeight()
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}

	}

	VkShaderModule VulkanInstance::createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}


	void VulkanInstance::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;

		Vec4 clearColorVal{ 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue clearColor = { {{clearColorVal.x,
									  clearColorVal.y,
									 clearColorVal.z,
									 clearColorVal.w}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicPipeline);


		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


		//binding vertex buffer
		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		//not possible to use different indices for each vertex attribute, so we do still have 
		//to completely duplicate vertex data even if just one attribute varies.
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16); //can be VK_INDEX_TYPE_UINT32

		// bind the right descriptor set for each frame to the descriptors in the shader 
		//with vkCmdBindDescriptorSets
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

		//this one is like opengl draw array
		//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		//this one is like opengl draw index
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);//3rd parameter instance count is for instancing

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
	void VulkanInstance::drawFrame(const WindowsWin& _Windows)
	{
		vkWaitForFences(m_logicalDevice, 1, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX);
		

		uint32_t imageIndex;
		if (VkResult result{ vkAcquireNextImageKHR(m_logicalDevice, m_SwapChain, UINT64_MAX, m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex) };
			result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain(_Windows);
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		updateUniformBuffer(m_currentFrame);

		// Only reset the fence if we are submitting work
		vkResetFences(m_logicalDevice, 1, &m_inFlightFence[m_currentFrame]);

		vkResetCommandBuffer(m_commandBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore[m_currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_graphicQueue, 1, &submitInfo, m_inFlightFence[m_currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		if (VkResult result{ vkQueuePresentKHR(m_presentQueue, &presentInfo) };
			result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain(_Windows);
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}

		//advance to the next frame
		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanInstance::updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		//ubo.model = Mat4(1.f); //* Mat4::Rotate(Vec3(0.f, 0.f, 1.f),  90.f);
		//ubo.view = Mat4(1.f);//Mat4::LookAt(Vec3(2.f, 2.f, 2.f), Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f));
		//ubo.proj = Mat4(1.f);//Mat4::Perspective(45.f * Mathf::Deg2Rad, 
					//static_cast<float>(m_swapChainExtent.width / m_swapChainExtent.height), 0.1f, 10.f);
		//ubo.proj.m[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

	}

	void VulkanInstance::recreateSwapChain(const WindowsWin& _Windows)
	{
		
		vkDeviceWaitIdle(m_logicalDevice); //should not touch resources that may still be in use


		//maybe touching on recreate the renderpass in the future, for now just make it simple

		cleanupSwapChain();

		createSwapChain(_Windows);
		createImageViews();
		createFrameBuffer();
	}

	void VulkanInstance::cleanupSwapChain()
	{
		//clean up framebuffers
		for (const auto& framebuffer : m_swapChainFramebuffers)
		{
			vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
		}

		//clean up imageView
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(m_logicalDevice, imageView, nullptr);
		}

		//clean up SwapChain
		vkDestroySwapchainKHR(m_logicalDevice, m_SwapChain, nullptr);
	}

	void VulkanInstance::createSwapChain(const WindowsWin& _Windows)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_PhysDeviceHandle);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, _Windows);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			imageCount = swapChainSupport.capabilities.maxImageCount;


		VkSwapchainCreateInfoKHR SwapChainInfo{};

		SwapChainInfo.sType = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		SwapChainInfo.pNext = { nullptr };
		SwapChainInfo.flags = { 0 };
		SwapChainInfo.surface = { m_Surface };
		SwapChainInfo.minImageCount = { imageCount };
		SwapChainInfo.imageFormat = { surfaceFormat.format };
		SwapChainInfo.imageColorSpace = { surfaceFormat.colorSpace };
		SwapChainInfo.imageExtent =
		{
		  { static_cast<decltype(VkExtent2D::width)>(_Windows.getWidth()) },
		  { static_cast<decltype(VkExtent2D::height)>(_Windows.getHeight()) }
		};
		SwapChainInfo.imageArrayLayers = { 1 };
		SwapChainInfo.imageUsage = { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT };

		QueueFamilyIndices indices = findQueueFamilies(m_PhysDeviceHandle);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			SwapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			SwapChainInfo.queueFamilyIndexCount = 2;
			SwapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			SwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}


		SwapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		SwapChainInfo.compositeAlpha = { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR };
		SwapChainInfo.presentMode = { presentMode };
		SwapChainInfo.clipped = VK_TRUE;
		SwapChainInfo.oldSwapchain = nullptr;  // reused here :^D

		if (vkCreateSwapchainKHR(m_logicalDevice, &SwapChainInfo, nullptr, &m_SwapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}
		vkGetSwapchainImagesKHR(m_logicalDevice, m_SwapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_logicalDevice, m_SwapChain, &imageCount, m_swapChainImages.data());

		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	void VulkanInstance::createImageViews()
	{
		swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void VulkanInstance::createFrameBuffer()
	{
		m_swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_swapChainExtent.width;
			framebufferInfo.height = m_swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	//use a host visible buffer as temp buffer and use a device local as actual vertex buffer
	void VulkanInstance::createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;


		createBuffers(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,//Buffer used as source in a memory transfer operation 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory);


		//Filling the vertex buffer
		void* data;
		vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

		createBuffers(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

		copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

		vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);

	}

	void VulkanInstance::createIndexBuffer()
	{
		VkDeviceSize buffersize = sizeof(m_indices[0]) * m_indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingbufferMemory;
		createBuffers(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer, stagingbufferMemory);


		void* data;
		vkMapMemory(m_logicalDevice, stagingbufferMemory, 0, buffersize, 0, &data);
		memcpy(data, m_indices.data(), (size_t)buffersize);
		vkUnmapMemory(m_logicalDevice, stagingbufferMemory);

		createBuffers(buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_IndexBufferMemory);

		copyBuffer(stagingBuffer, m_indexBuffer, buffersize);

		vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_logicalDevice, stagingbufferMemory, nullptr);

	}

	//uniformbuffers
	void VulkanInstance::createUniformBuffers()
	{
		VkDeviceSize buffersize = sizeof(UniformBufferObject);
		//we don't want to update the buffer in preparation of the next frame while a previous one is still reading from it! 
		//Thus, we need to have as many uniform buffers as we have frames in flight, 
		//and write to a uniform buffer that is not currently being read by the GPU
		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; i++)
		{

			createBuffers(buffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(m_logicalDevice, uniformBuffersMemory[i], 0, buffersize, 0, &uniformBuffersMapped[i]);

		}

	}

	uint32_t VulkanInstance::findMemoryType(const uint32_t& typeFiler, VkMemoryPropertyFlags properties)
	{
		//query info about the available types of memory 
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysDeviceHandle, &memProperties);

		for (uint32_t i{ 0 }; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFiler & (1 << i)) && 
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	void VulkanInstance::createBuffers(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
									   VkDeviceMemory& buffermemory)
	{

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType		= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size			= size;									 //size of buffer in bytes
		bufferInfo.usage		= usage;								 //use for other usage cause use | (other usage)
		bufferInfo.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;			 //can own by a specific queue family but for now graphics queue only
		bufferInfo.flags		= 0;									 // to configure sparse buffer memory not relevant now

		if (vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &buffer))
		{
			throw std::runtime_error("failed to create vertex buffer!");
		}


		//Memory Requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_logicalDevice, buffer, &memRequirements);

		//Memory Allocation
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		//The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a 
		// single allocation among many different objects by using the offset parameters that we've seen in many functions.
		//You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative
	
		if (vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &buffermemory) != VK_SUCCESS)//cant call this everytime  
		{
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(m_logicalDevice, buffer, buffermemory, 0);
	}


	//copy contents from one buffer to another
	void VulkanInstance::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		//command buffers execute memory transfer operation 
		//You may wish to create a separate command pool for these kinds of short-lived 
		//buffers, because the implementation may be able to apply memory allocation optimizations. You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool generation in that case.
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;                   
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);

		//using command buffer once and wait with returning from the function until 
		// the copy operation has finished executing
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

			//only contains the copy command
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0; //optional
			copyRegion.dstOffset = 0; //optional
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		//2 method to wait for transfer to complete
		//1. vkQueuWaitIdle
		//2. vkWaitForFences // allow to schedule multiple transfer simultaneously and wait for all to complete instead of executing one at a time
		vkQueueSubmit(m_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_graphicQueue);

		//clean up command buffer
		vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &commandBuffer);

	}
	void VulkanInstance::createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0; //location 0?
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // what we describing now is a uniform buffer?
		uboLayoutBinding.descriptorCount = 1; //one uniform buffer for now
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// referencing the descriptor from the vertex shader for now but can putcan be a combination of VkShaderStageFlagBits values  or VK_SHADER_STAGE_ALL_GRAPHICS  
		uboLayoutBinding.pImmutableSamplers = nullptr; //optional (only relevant for image sampling

		//create descriptor layout
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings	= &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}

	}
	void VulkanInstance::createDescriptorPool()
	{
		//describe which descriptor types our descriptor sets are going to contain and how many of them
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		//allocate one of these descriptors for every frame
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;

		//specify the maximum number of descriptor sets that may be allocated
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(m_logicalDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

	}

	//descriptor pool to allocate from, the number of descriptor sets to allocate, 
	//and the descriptor layout to base them on
	void VulkanInstance::createDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(m_logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(m_logicalDevice, 1, &descriptorWrite, 0, nullptr);
		}
	}

}//namespace TDS