#include "Application.h"
#include <set>
#include <algorithm>
#include <fstream>

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtenstion =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static std::vector<char> ReadFile(std::string_view fileName)
{
	std::vector<char> data;

	std::ifstream in(fileName.data(), std::ios::ate | std::ios::binary);
	if (!in.is_open())
	{
		std::cerr << "Failed open file : " << fileName.data() << std::endl;
		return data;
	}

	size_t size = (size_t)in.tellg();
	data.resize(size);
	in.seekg(0);
	in.read(data.data(), size);

	in.close();

	return data;
}

VulkanApp::VulkanApp(const std::string_view title, int width, int height)
	: mWinWidth(width), mWinHeight(height)
{
	InitWindow(title, width, height);
}

VulkanApp::~VulkanApp()
{

}

void VulkanApp::Run()
{
	InitVulkan();
	MainLoop();
	Close();
}

void VulkanApp::InitWindow(const std::string_view title, int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//不创建OpenGL上下文

	m_pWindow = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
}

void VulkanApp::InitVulkan()
{
	CreateVulkanInstance();
	SetupDebugCallback();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicDevice();
	CreateSwapChain();
	CreateSwapChainImageView();
	CreateGraphicsPipeline();
}

void VulkanApp::MainLoop()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
	}
}

void VulkanApp::Close()
{
	vkDestroyShaderModule(m_pDevice, m_pVertexShaderModule, nullptr);
	vkDestroyShaderModule(m_pDevice, m_pFragmentShaderModule, nullptr);
	for (auto& imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(m_pDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_pDevice, m_pSwapChain, nullptr);
	vkDestroyDevice(m_pDevice, nullptr);
	vkDestroySurfaceKHR(m_pVKInstance, m_pSurface, nullptr);
	DestroyDebugUtilsMessengerEXT();
	vkDestroyInstance(m_pVKInstance, nullptr);

	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

void VulkanApp::CreateVulkanInstance()
{
	if (!CheckValidationLayerSupport())
	{
		std::cout << "Validation layers required, but not available" << std::endl;
		return;
	}

	VkApplicationInfo info		= {};
	info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pApplicationName		= "Hello_Triangle";
	info.applicationVersion		= VK_MAKE_API_VERSION(0, 1, 2, 0);
	info.pEngineName			= "No_Engine";
	info.engineVersion			= VK_MAKE_API_VERSION(0, 1, 2, 0);
	info.apiVersion				= VK_API_VERSION_1_2;

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> ExtensionsProp(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, ExtensionsProp.data());

	std::vector<const char*> Extensions = GetRequireExtenstion();

	VkInstanceCreateInfo createInfo		= {};
	createInfo.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo			= &info;
	createInfo.ppEnabledExtensionNames  = Extensions.data();
	createInfo.enabledExtensionCount	= static_cast<uint32_t>(Extensions.size());
	createInfo.enabledLayerCount		= static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames		= validationLayers.data();

	std::cout << "Extension count: " << extensionCount << std::endl;
	for (const auto& p : ExtensionsProp)
	{
		std::cout << "\t Extension name: " << p.extensionName << std::endl;
	}
	std::cout << "Enable Extension count: " << createInfo.enabledExtensionCount << std::endl;
	for (uint32_t i = 0; i < createInfo.enabledExtensionCount; i++)
	{
		std::cout << "\t Enable Extension name: " << createInfo.ppEnabledExtensionNames[i] << std::endl;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_pVKInstance) != VK_SUCCESS)
	{
		std::cout << "VKInstance create failed" << std::endl;
	}
}

bool VulkanApp::CheckValidationLayerSupport() const
{
	uint32_t count = 0;
	vkEnumerateInstanceLayerProperties(&count, nullptr);
	std::vector<VkLayerProperties> Layers (count);
	vkEnumerateInstanceLayerProperties(&count, Layers.data());

	for (const char* layerName : validationLayers)
	{
		bool found = false;
		for (const auto& layer : Layers)
		{
			if (strcmp(layerName, layer.layerName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanApp::GetRequireExtenstion() const
{
	uint32_t glfwExtenstionCount = 0;
	const char** glfwExtenstions = glfwGetRequiredInstanceExtensions(&glfwExtenstionCount);

	std::vector<const char*> Extenstions(glfwExtenstions, glfwExtenstions + glfwExtenstionCount);

	//if debug
	Extenstions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return Extenstions;
}

void VulkanApp::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_pVKInstance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		std::cerr << "No physical device support" << std::endl;
		return;
	}

	std::vector<VkPhysicalDevice> Devices(deviceCount);
	vkEnumeratePhysicalDevices(m_pVKInstance, &deviceCount, Devices.data());

	for (const auto& device : Devices)
	{
		if (IsDeviceSuitable(device))
		{
			m_pPhysicalDevice = device;
			break;
		}
	}

	if (m_pPhysicalDevice == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to find suitable GPU" << std::endl;
		return;
	}
}

bool VulkanApp::IsDeviceSuitable(VkPhysicalDevice pDevice) const
{
	QueueFamilyIndex index = FindQueueFamilies(pDevice);

	bool extenstionSupport = CheckDeviceExtenstionSupport(pDevice);

	bool swapChainAdequate = false;
	if (extenstionSupport)
	{
		SwapChainSupportDetails details = QuerySwapChainSupport(pDevice);
		swapChainAdequate = !details.formates.empty() && !details.presentModes.empty();
	}
	return index.IsComplete() && extenstionSupport && swapChainAdequate;
}

QueueFamilyIndex VulkanApp::FindQueueFamilies(VkPhysicalDevice pDevice) const
{
	uint32_t familiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &familiesCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(familiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &familiesCount, QueueFamilies.data());

	QueueFamilyIndex index;
	int i = 0;
	for (const auto& family : QueueFamilies)
	{
		if (family.queueCount > 0 && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			index.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, m_pSurface, &presentSupport);
		if (presentSupport)
		{
			index.presentFamily = i;
		}

		if (index.IsComplete())
		{
			break;
		}

		i++;
	}

	return index;
}

void VulkanApp::CreateLogicDevice()
{
	QueueFamilyIndex index = FindQueueFamilies(m_pPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::set<int32_t> UniqueQueueFamilise = { index.graphicsFamily, index.presentFamily };

	float priority = 1.f;
	for (int32_t index : UniqueQueueFamilise)
	{
		VkDeviceQueueCreateInfo createInfo	= {};
		createInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.queueCount				= 1;
		createInfo.queueFamilyIndex			= index;
		createInfo.pQueuePriorities			= &priority;
		QueueCreateInfos.emplace_back(createInfo);
	}

	VkPhysicalDeviceFeatures features = {};

	VkDeviceCreateInfo deviceCreateInfo			= {};
	deviceCreateInfo.sType						= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos			= QueueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount		= static_cast<uint32_t>(QueueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures			= &features;
	deviceCreateInfo.enabledLayerCount			= static_cast<uint32_t>(validationLayers.size());
	deviceCreateInfo.ppEnabledLayerNames		= validationLayers.data();
	deviceCreateInfo.enabledExtensionCount		= static_cast<uint32_t>(deviceExtenstion.size());
	deviceCreateInfo.ppEnabledExtensionNames	= deviceExtenstion.data();

	if (vkCreateDevice(m_pPhysicalDevice, &deviceCreateInfo, nullptr, &m_pDevice) != VK_SUCCESS)
	{
		std::cerr << "VKDevice create failed" << std::endl;
	}

	vkGetDeviceQueue(m_pDevice, index.graphicsFamily, 0, &m_pGraphicQueue);
	vkGetDeviceQueue(m_pDevice, index.presentFamily, 0, &m_pPresentQueue);

}

void VulkanApp::SetupDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	info.pfnUserCallback = DebugCallback;
	info.pUserData = nullptr;

	if (CreateDebugUtilsMessengerEXT(m_pVKInstance, &info, nullptr, &m_pDebugUtils) != VK_SUCCESS)
	{
		std::cerr << "falied to setup debug callback" << std::endl;
	}
}

VkResult VulkanApp::CreateDebugUtilsMessengerEXT(VkInstance pInstance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* ppCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(pInstance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(pInstance, pCreateInfo, pAllocator, ppCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanApp::DestroyDebugUtilsMessengerEXT()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_pVKInstance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(m_pVKInstance, m_pDebugUtils, nullptr);
	}
}

void VulkanApp::CreateSurface()
{
	if (glfwCreateWindowSurface(m_pVKInstance, m_pWindow, nullptr, &m_pSurface) != VK_SUCCESS)
	{
		std::cerr << "VkSurfaceKHR create failed" << std::endl;
	}
}

bool VulkanApp::CheckDeviceExtenstionSupport(VkPhysicalDevice pDevice) const
{
	uint32_t extenstionCount = 0;
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extenstionCount, nullptr);
	std::vector<VkExtensionProperties> ExtenstionProps(extenstionCount);
	vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extenstionCount, ExtenstionProps.data());
	std::set<std::string> RequireExtenstion(deviceExtenstion.begin(), deviceExtenstion.end());

	for (const auto& extenstion : ExtenstionProps)
	{
		RequireExtenstion.erase(extenstion.extensionName);
	}
	return RequireExtenstion.empty();
}

SwapChainSupportDetails VulkanApp::QuerySwapChainSupport(VkPhysicalDevice pDevice) const
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice, m_pSurface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, m_pSurface, &formatCount, nullptr);
	if (formatCount > 0)
	{
		details.formates.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, m_pSurface, &formatCount, details.formates.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, m_pSurface, &presentModeCount, nullptr);
	if (presentModeCount > 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, m_pSurface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanApp::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	
	if (availableFormats.size() >= 1)
	{
		return availableFormats[0];
	}

	return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR VulkanApp::ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availabePresentModes) const
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& mode : availabePresentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return mode;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = mode;
		}
	}
	return bestMode;
}

VkExtent2D VulkanApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
	//一些窗口系统会使用一个特殊值，扵扩扮扴戳戲 扴变量类型的构大值，表示允许我们自己选择对于窗口柣合适的交换范围，
	//但我们选择的交换范围韴要在minImageExtent与maxImageExtent的范围内
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D extent2D = { mWinWidth, mWinHeight };
		extent2D.width		= std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent2D.width));
		extent2D.height		= std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent2D.height));

		return extent2D;
	}
}

void VulkanApp::CreateSwapChain()
{
	SwapChainSupportDetails details = QuerySwapChainSupport(m_pPhysicalDevice);
	VkSurfaceFormatKHR format		= ChooseSurfaceFormat(details.formates);
	VkPresentModeKHR presentMode	= ChooseSwapChainPresentMode(details.presentModes);
	VkExtent2D extent				= ChooseSwapExtent(details.capabilities);
	uint32_t imageCoun				= details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && imageCoun > details.capabilities.maxImageCount)
	{
		imageCoun = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface					= m_pSurface;
	createInfo.minImageCount			= imageCoun;
	createInfo.imageFormat				= format.format;
	createInfo.imageColorSpace			= format.colorSpace;
	createInfo.imageExtent				= extent;
	createInfo.imageArrayLayers			= 1;
	createInfo.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.presentMode				= presentMode;
	createInfo.clipped					= true;

	QueueFamilyIndex index			= FindQueueFamilies(m_pPhysicalDevice);
	uint32_t queueFamilyIndices[]	= { index.graphicsFamily, index.presentFamily };
	if (index.graphicsFamily != index.presentFamily)
	{
		createInfo.queueFamilyIndexCount	= 2;
		createInfo.pQueueFamilyIndices		= queueFamilyIndices;
		createInfo.imageSharingMode			= VK_SHARING_MODE_CONCURRENT;
	}
	else
	{
		createInfo.queueFamilyIndexCount	= 0;
		createInfo.pQueueFamilyIndices		= nullptr;
		createInfo.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform		= details.capabilities.currentTransform;
	createInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.oldSwapchain		= VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_pDevice, &createInfo, nullptr, &m_pSwapChain) != VK_SUCCESS)
	{
		std::cerr << "VkSwapchainKHR create failed" << std::endl;
		return;
	}

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(m_pDevice, m_pSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_pDevice, m_pSwapChain, &imageCount, mSwapChainImages.data());

	mSwapChainImageFormat = format.format;
	mSwapChainImageExtent = extent;
}

void VulkanApp::CreateSwapChainImageView()
{
	mSwapChainImageViews.resize(mSwapChainImages.size());
	for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = mSwapChainImages[i];
		createInfo.format = mSwapChainImageFormat;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_pDevice, &createInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
		{
			std::cerr << "VkImageView " << i << " create failed" << std::endl;
			return;
		}
	}
}

void VulkanApp::CreateGraphicsPipeline()
{
	std::vector<char> vertexShaderCode = ReadFile("shader/vert.spv");
	std::vector<char> fragmentShaderCode = ReadFile("shader/frag.spv");
	m_pVertexShaderModule = CreateShaderModule(vertexShaderCode);
	m_pFragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	VkPipelineShaderStageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.module = m_pVertexShaderModule;
	createInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	createInfo.pName = "main";
	createInfo.pSpecializationInfo = nullptr;

	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
	vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateInfo.pVertexAttributeDescriptions = nullptr;
	vertexInputStateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateInfo.pVertexBindingDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {};
	inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewPort = {};
	viewPort.x = 0.f;
	viewPort.y = 0.f;
	viewPort.width = (float)mSwapChainImageExtent.width;
	viewPort.height = (float)mSwapChainImageExtent.height;
	viewPort.minDepth = 0.f;
	viewPort.maxDepth = 1.f;

	VkRect2D rect = {};
	rect.offset = { 0, 0 };
	rect.extent = mSwapChainImageExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pViewports = &viewPort;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pScissors = &rect;
	viewportStateCreateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
	rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateInfo.lineWidth = 1.f;
	rasterizationStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateInfo.depthBiasClamp = 0.f;
	rasterizationStateInfo.depthBiasConstantFactor = 0.f;
	rasterizationStateInfo.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
	multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateInfo.minSampleShading = 0.f;
	multisampleStateInfo.pSampleMask = nullptr;
	multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateInfo.alphaToOneEnable = VK_FALSE;

}

VkShaderModule VulkanApp::CreateShaderModule(const std::vector<char>& shaderCode) const
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	VkShaderModule pShader = VK_NULL_HANDLE;
	if (vkCreateShaderModule(m_pDevice, &createInfo, nullptr, &pShader) != VK_SUCCESS)
	{
		std::cerr << "VkShaderModule create failed" << std::endl;
	}
	return pShader;
}
