#include "Application.h"
#include <set>
#include <algorithm>
#include <fstream>
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtenstion =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const static std::vector<VertexData> g_Vertices = 
{
	{{-0.5f, -0.5f}, {1.f, 0.f, 0.f} },
	{{ 0.5f, -0.5f}, {0.f, 1.f, 0.f} },
	{{ 0.5f,  0.5f}, {0.f, 0.f, 1.f} },
	{{-0.5f,  0.5f}, {1.f, 1.f, 0.f} }
};

const static std::vector<uint16_t> g_Indices =
{
	0, 1, 2, 2, 3, 0
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
	: mWinWidth(width), mWinHeight(height), mCurrFrame(0), mFramebufferResized(false)
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
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	m_pWindow = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_pWindow, this);

	glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int width, int height)
	{
		auto app = (VulkanApp*)glfwGetWindowUserPointer(pWindow);
		app->SetFramebufferResize(true);
	});
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
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFrameBuffer();
	CreateCommandPool();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSemaphores();
}

void VulkanApp::MainLoop()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(m_pDevice);
}

void VulkanApp::Close()
{
	for (int i = 0; i < 2; i++)
	{
		vkDestroySemaphore(m_pDevice, mImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_pDevice, mImageFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_pDevice, mInFlightFences[i], nullptr);
	}

	CleanupSwapChain();

	vkDestroyDescriptorPool(m_pDevice, m_pDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_pDevice, m_pDescriptorSetLayout, nullptr);
	for (size_t i = 0; i < mUniformBuffers.size(); i++)
	{
		vkDestroyBuffer(m_pDevice, mUniformBuffers[i], nullptr);
		vkFreeMemory(m_pDevice, mUniformBuffersMemory[i], nullptr);
	}

	vkDestroyBuffer(m_pDevice, m_pIndexBuffer, nullptr);
	vkFreeMemory(m_pDevice, m_pIndexBufferMemory, nullptr);
	vkDestroyBuffer(m_pDevice, m_pVertexBuffer, nullptr);
	vkFreeMemory(m_pDevice, m_pVertexBufferMemory, nullptr);
	vkDestroyCommandPool(m_pDevice, m_pCommandPool, nullptr);
	
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
	info.applicationVersion		= VK_MAKE_API_VERSION(0, 1, 3, 0);
	info.pEngineName			= "No_Engine";
	info.engineVersion			= VK_MAKE_API_VERSION(0, 1, 3, 0);
	info.apiVersion				= VK_API_VERSION_1_3;

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
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
		int width, height = 0;
		glfwGetFramebufferSize(m_pWindow, &width, &height);
		VkExtent2D extent2D = { width, height };
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

void VulkanApp::RecreateSwapChain()
{
	int width, height = 0;
	glfwGetFramebufferSize(m_pWindow, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_pWindow, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(m_pDevice);

	CleanupSwapChain();

	CreateSwapChain();
	CreateSwapChainImageView();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffer();
	CreateCommandBuffers();
}

void VulkanApp::CleanupSwapChain()
{
	for (auto& framebuffer : mFrameBuffers)
	{
		vkDestroyFramebuffer(m_pDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_pDevice, m_pCommandPool, (uint32_t)mCommandBuffers.size(), mCommandBuffers.data());

	vkDestroyPipeline(m_pDevice, m_pPipeline, nullptr);
	vkDestroyPipelineLayout(m_pDevice, m_pPipelineLayout, nullptr);
	vkDestroyRenderPass(m_pDevice, m_pRenderPass, nullptr);
	for (auto& imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(m_pDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_pDevice, m_pSwapChain, nullptr);
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
	VkShaderModule pVertexShaderModule = CreateShaderModule(vertexShaderCode);
	VkShaderModule pFragmentShaderModule = CreateShaderModule(fragmentShaderCode);
	
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.module = pVertexShaderModule;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.pName = "main";
	vertexShaderCreateInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.module = pFragmentShaderModule;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.pName = "main";
	fragmentShaderCreateInfo.pSpecializationInfo = nullptr;
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
	vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto descriptions = VertexData::GetVertexInputAttributeDescriptions();
	vertexInputStateInfo.vertexAttributeDescriptionCount = (uint32_t)descriptions.size();
	vertexInputStateInfo.pVertexAttributeDescriptions = descriptions.data();
	vertexInputStateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateInfo.pVertexBindingDescriptions = &VertexData::GetBindingDescription();

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

	VkPipelineColorBlendAttachmentState colorBlendState = {};
	colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendState.blendEnable = VK_FALSE;
	/*colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
*/
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.f;

	VkDynamicState dynamicState[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicState;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_pDescriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_pDevice, &pipelineLayoutCreateInfo, nullptr, &m_pPipelineLayout) != VK_SUCCESS)
	{
		std::cerr << "VkPipelineLayout create failed" << std::endl;
		return;
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	//graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateInfo;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.renderPass = m_pRenderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.layout = m_pPipelineLayout;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_pDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_pPipeline) != VK_SUCCESS)
	{
		std::cerr << "VkPipeline create failed" << std::endl;
		return;
	}

	vkDestroyShaderModule(m_pDevice, pVertexShaderModule, nullptr);
	vkDestroyShaderModule(m_pDevice, pFragmentShaderModule, nullptr);
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

void VulkanApp::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = mSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependencyInfo = {};
	dependencyInfo.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencyInfo.dstSubpass = 0;
	dependencyInfo.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencyInfo.srcAccessMask = 0;
	dependencyInfo.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencyInfo.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderCreateInfo = {};
	renderCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderCreateInfo.pAttachments = &colorAttachment;
	renderCreateInfo.attachmentCount = 1;
	renderCreateInfo.subpassCount = 1;
	renderCreateInfo.pSubpasses = &subpass;
	renderCreateInfo.dependencyCount = 1;
	renderCreateInfo.pDependencies = &dependencyInfo;

	if (vkCreateRenderPass(m_pDevice, &renderCreateInfo, nullptr, &m_pRenderPass) != VK_SUCCESS)
	{
		std::cerr << "VkRenderPass create failed" << std::endl;
	}
}

void VulkanApp::CreateFrameBuffer()
{
	mFrameBuffers.resize(mSwapChainImageViews.size());

	for (size_t i = 0; i < mFrameBuffers.size(); i++)
	{
		VkImageView attachments[] = { mSwapChainImageViews[i] };
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = mSwapChainImageExtent.width;
		createInfo.height = mSwapChainImageExtent.height;
		createInfo.renderPass = m_pRenderPass;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_pDevice, &createInfo, nullptr, &mFrameBuffers[i]) != VK_SUCCESS)
		{
			std::cerr << "VkFramebuffer " << i << " create failed" << std::endl;
			return;
		}
	}
}

void VulkanApp::CreateCommandPool()
{
	QueueFamilyIndex index = FindQueueFamilies(m_pPhysicalDevice);

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = index.graphicsFamily;

	if (vkCreateCommandPool(m_pDevice, &createInfo, nullptr, &m_pCommandPool) != VK_SUCCESS)
	{
		std::cerr << "VkCommandPool create failed" << std::endl;
		return;
	}
}

void VulkanApp::CreateCommandBuffers()
{
	mCommandBuffers.resize(mFrameBuffers.size());
	
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = m_pCommandPool;
	info.commandBufferCount = (uint32_t)mCommandBuffers.size();
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_pDevice, &info, mCommandBuffers.data()) != VK_SUCCESS)
	{
		std::cerr << "VkCommandBuffer create failed" << std::endl;
		return;
	}
}

void VulkanApp::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	info.pInheritanceInfo = nullptr;
	
	if (vkBeginCommandBuffer(commandBuffer, &info) != VK_SUCCESS)
	{
		std::cerr << "vkBeginCommandBuffer failed" << std::endl;
		return;
	}
	

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = m_pRenderPass;
	VkRect2D renderArea = {};
	renderArea.extent = mSwapChainImageExtent;
	renderArea.offset = { 0, 0 };
	beginInfo.renderArea = renderArea;
	VkClearValue clearValue = {};
	clearValue.color = { 0.f, 0.f, 0.f, 1.f };
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clearValue;
	beginInfo.framebuffer = mFrameBuffers[imageIndex];

	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE); //VK_SUBPASS_CONTENTS_INLINE : 所有要执行的指令都在主要指令缓冲中，没有辅助指令缓冲需要执行

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mSwapChainImageExtent.width;
	viewport.height = (float)mSwapChainImageExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	//vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapChainImageExtent;
	//vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { m_pVertexBuffer };
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &offset);
	vkCmdBindIndexBuffer(commandBuffer, m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &mDescriptorSets[imageIndex], 0, nullptr);
	
	//vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		std::cerr << "vkEndCommandBuffer failed" << std::endl;
	}
}

void VulkanApp::CreateSemaphores()
{
	mImageAvailableSemaphores.resize(2);
	mImageFinishedSemaphores.resize(2);
	mInFlightFences.resize(2);

	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < 2; i++)
	{
		if (vkCreateSemaphore(m_pDevice, &createInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_pDevice, &createInfo, nullptr, &mImageFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_pDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
		{
			std::cerr << "VkSemaphore create failed" << std::endl;
		}
	}
}

void VulkanApp::DrawFrame()
{
	vkWaitForFences(m_pDevice, 1, &mInFlightFences[mCurrFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(m_pDevice, m_pSwapChain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphores[mCurrFrame], nullptr, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//mFramebufferResized = false;
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		assert(false);
	}

	vkResetCommandBuffer(mCommandBuffers[imageIndex], 0);
	RecordCommandBuffer(mCommandBuffers[imageIndex], imageIndex);

	UpdateUniformBuffer(imageIndex);

	//提交指令缓冲
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &mCommandBuffers[imageIndex];
	VkSemaphore semaphores[] = { mImageAvailableSemaphores[mCurrFrame] };
	VkPipelineStageFlags stageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//与semaphores的索引相对应
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = semaphores;
	info.pWaitDstStageMask = stageFlags;
	VkSemaphore signalSemaphores[] = { mImageFinishedSemaphores[mCurrFrame] };
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_pDevice, 1, &mInFlightFences[mCurrFrame]);

	if (vkQueueSubmit(m_pGraphicQueue, 1, &info, mInFlightFences[mCurrFrame]) != VK_SUCCESS)
	{
		std::cerr << "GraphicQueue submit failed" << std::endl;
	}

	//呈现
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_pSwapChain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.pResults = nullptr;
	VkResult pr_result = vkQueuePresentKHR(m_pPresentQueue, &presentInfo);
	if (pr_result == VK_ERROR_OUT_OF_POOL_MEMORY || pr_result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
	{
		mFramebufferResized = false;
		RecreateSwapChain();
	}
	else if (pr_result != VK_SUCCESS)
	{
		std::cerr << "QueuePresentKHR failed" << std::endl;
		assert(0);
	}

	mCurrFrame = (mCurrFrame + 1) % 2;
}

void VulkanApp::CreateVertexBuffer()
{
	/*float vertices[] = {
		0.0, -0.5,
		0.5, 0.5,
		-0.5, 0.5
	};*/

	VkDeviceSize size = sizeof(g_Vertices[0]) * g_Vertices.size();

	VkBuffer pTempBuffer;
	VkDeviceMemory pTempMemory;
	CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, pTempBuffer, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pTempMemory);

	void* pData;
	vkMapMemory(m_pDevice, pTempMemory, 0, size, 0, &pData);
	memcpy(pData, g_Vertices.data(), size);
	vkUnmapMemory(m_pDevice, pTempMemory);

	CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size, m_pVertexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_pVertexBufferMemory);

	CopyBuffer(pTempBuffer, m_pVertexBuffer, size);

	vkDestroyBuffer(m_pDevice, pTempBuffer, nullptr);
	vkFreeMemory(m_pDevice, pTempMemory, nullptr);
}

void VulkanApp::CreateIndexBuffer()
{
	VkDeviceSize size = sizeof(g_Indices[0]) * g_Indices.size();

	VkBuffer pTempBuffer;
	VkDeviceMemory pTempMemory;
	CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, pTempBuffer, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pTempMemory);

	void* pData;
	vkMapMemory(m_pDevice, pTempMemory, 0, size, 0, &pData);
	memcpy(pData, g_Indices.data(), size);
	vkUnmapMemory(m_pDevice, pTempMemory);

	CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size, m_pIndexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_pIndexBufferMemory);

	CopyBuffer(pTempBuffer, m_pIndexBuffer, size);

	vkDestroyBuffer(m_pDevice, pTempBuffer, nullptr);
	vkFreeMemory(m_pDevice, pTempMemory, nullptr);
}

void VulkanApp::CreateBuffer(VkBufferUsageFlags usage, VkDeviceSize size, VkBuffer& pBuffer, VkMemoryPropertyFlags Property, VkDeviceMemory& pMemory)
{
	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = usage;
	info.size = size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_pDevice, &info, nullptr, &pBuffer) != VK_SUCCESS)
	{
		assert(0);
	}

	VkMemoryRequirements memoryRequirement;
	vkGetBufferMemoryRequirements(m_pDevice, pBuffer, &memoryRequirement);

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirement.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirement.memoryTypeBits, Property);

	if (vkAllocateMemory(m_pDevice, &allocateInfo, nullptr, &pMemory) != VK_SUCCESS)
	{
		assert(0);
	}

	vkBindBufferMemory(m_pDevice, pBuffer, pMemory, 0);
}

uint32_t VulkanApp::FindMemoryType(uint32_t fliter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties deviceMemoryPrpperties;
	vkGetPhysicalDeviceMemoryProperties(m_pPhysicalDevice, &deviceMemoryPrpperties);
	for (uint32_t i = 0; i < deviceMemoryPrpperties.memoryTypeCount; i++)
	{
		if ((deviceMemoryPrpperties.memoryTypes[i].propertyFlags &properties) == properties && (fliter & (1 << i)))
		{
			return i;
		}
	}
	assert(0);
	return 0;
}

void VulkanApp::CopyBuffer(VkBuffer pSrcBuffer, VkBuffer pDstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;
	info.commandPool = m_pCommandPool;

	VkCommandBuffer pCommandBuffer;
	vkAllocateCommandBuffers(m_pDevice, &info, &pCommandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(pCommandBuffer, &beginInfo);
	{
		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = size;

		vkCmdCopyBuffer(pCommandBuffer, pSrcBuffer, pDstBuffer, 1, &bufferCopy);
	}
	vkEndCommandBuffer(pCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &pCommandBuffer;
	vkQueueSubmit(m_pGraphicQueue, 1, &submitInfo, nullptr);
	vkQueueWaitIdle(m_pGraphicQueue);
	vkFreeCommandBuffers(m_pDevice, m_pCommandPool, 1, &pCommandBuffer);
}

void VulkanApp::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	VkDescriptorSetLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = 1;
	info.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(m_pDevice, &info, nullptr, &m_pDescriptorSetLayout) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanApp::CreateUniformBuffers()
{
	VkDeviceSize size = sizeof(UniformBufferObj);
	mUniformBuffers.resize(mSwapChainImages.size());
	mUniformBuffersMemory.resize(mSwapChainImages.size());
	for (size_t i = 0; i < mUniformBuffers.size(); i++)
	{
		CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, mUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffersMemory[i]);
	}
}

void VulkanApp::UpdateUniformBuffer(uint32_t imageIndex)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currTime - startTime).count();

	UniformBufferObj ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.f), glm::radians(90.f) * time, { 0.f, 0.f, 1.f });
	ubo.view = glm::lookAt(glm::vec3( 2.f, 2.f, 2.f ), glm::vec3( 0.f, 0.f, 0.f ), glm::vec3( 0.f, 0.f, 1.f ));
	ubo.proj = glm::perspective(glm::radians(45.f), (float)mSwapChainImageExtent.width / mSwapChainImageExtent.height, 0.1f, 1000.f);
	//ubo.proj[1][1] *= -1;

	void* pData;
	vkMapMemory(m_pDevice, mUniformBuffersMemory[imageIndex], 0, sizeof(ubo), 0, &pData);
	memcpy(pData, &ubo, sizeof(ubo));
	vkUnmapMemory(m_pDevice, mUniformBuffersMemory[imageIndex]);
}

void VulkanApp::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.descriptorCount = (uint32_t)mSwapChainImages.size();
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = 1;
	info.pPoolSizes = &poolSize;
	info.maxSets = (uint32_t)mSwapChainImages.size();

	if (vkCreateDescriptorPool(m_pDevice, &info, nullptr, &m_pDescriptorPool) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanApp::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(mSwapChainImages.size(), m_pDescriptorSetLayout);
	VkDescriptorSetAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorSetCount = (uint32_t)layouts.size();
	info.pSetLayouts = layouts.data();
	info.descriptorPool = m_pDescriptorPool;

	mDescriptorSets.resize(mSwapChainImages.size());
	if (vkAllocateDescriptorSets(m_pDevice, &info, mDescriptorSets.data()) != VK_SUCCESS)
	{
		assert(0);
	}

	for (size_t i = 0; i < mDescriptorSets.size(); i++)
	{
		VkDescriptorBufferInfo info = {};
		info.buffer = mUniformBuffers[i];
		info.offset = 0;
		info.range = sizeof(UniformBufferObj);

		VkWriteDescriptorSet writeSet = {};
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet = mDescriptorSets[i];
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = 0;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeSet.descriptorCount = 1;
		writeSet.pBufferInfo = &info;
		vkUpdateDescriptorSets(m_pDevice, 1, &writeSet, 0, nullptr);
	}
}
