#include "DeferredApp.h"
#include <set>
#include <algorithm>
#include <fstream>
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

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
	//前
	{{-0.5f, -0.5f, 0.5f}, {1.f, 0.f, 0.f}, {0.f, 0.f} },
	{{ 0.5f, -0.5f, 0.5f}, {0.f, 1.f, 0.f}, {1.f, 0.f} },
	{{ 0.5f,  0.5f, 0.5f}, {0.f, 0.f, 1.f}, {1.f, 1.f} },
	{{-0.5f,  0.5f, 0.5f}, {1.f, 1.f, 0.f}, {0.f, 1.f} },

	//后
	{{ 0.5f, -0.5f, -0.5f}, {0.f, 1.f, 0.f}, {0.f, 0.f} },
	{{-0.5f, -0.5f, -0.5f}, {1.f, 0.f, 0.f}, {1.f, 0.f} },
	{{-0.5f,  0.5f, -0.5f}, {1.f, 1.f, 0.f}, {1.f, 1.f} },
	{{ 0.5f,  0.5f, -0.5f}, {0.f, 0.f, 1.f}, {0.f, 1.f} },

	//左
	{{-0.5f,  -0.5f,  0.5f}, {1.f, 1.f, 0.f}, {1.f, 0.f} },
	{{-0.5f,   0.5f,  0.5f}, {1.f, 1.f, 0.f}, {1.f, 1.f} },
	{{-0.5f,   0.5f, -0.5f}, {1.f, 1.f, 0.f}, {0.f, 1.f} },
	{{-0.5f,  -0.5f, -0.5f}, {1.f, 1.f, 0.f}, {0.f, 0.f} },

	//右
	{{0.5f,  -0.5f,  0.5f}, {1.f, 1.f, 0.f}, {0.f, 0.f} },
	{{0.5f,  -0.5f, -0.5f}, {1.f, 1.f, 0.f}, {1.f, 0.f} },
	{{0.5f,   0.5f, -0.5f}, {1.f, 1.f, 0.f}, {1.f, 1.f} },
	{{0.5f,   0.5f,  0.5f}, {1.f, 1.f, 0.f}, {0.f, 1.f} },

	//上
	{{-0.5f,  -0.5f, -0.5f}, {1.f, 1.f, 0.f}, {0.f, 0.f} },
	{{ 0.5f,  -0.5f, -0.5f}, {1.f, 1.f, 0.f}, {1.f, 0.f} },
	{{ 0.5f,  -0.5f,  0.5f}, {1.f, 1.f, 0.f}, {1.f, 1.f} },
	{{-0.5f,  -0.5f,  0.5f}, {1.f, 1.f, 0.f}, {0.f, 1.f} },

	//下
	{{-0.5f,   0.5f,  0.5f}, {1.f, 1.f, 0.f}, {0.f, 0.f} },
	{{ 0.5f,   0.5f,  0.5f}, {1.f, 1.f, 0.f}, {1.f, 0.f} },
	{{ 0.5f,   0.5f, -0.5f}, {1.f, 1.f, 0.f}, {1.f, 1.f} },
	{{-0.5f,   0.5f, -0.5f}, {1.f, 1.f, 0.f}, {0.f, 1.f} },
};

const static std::vector<uint16_t> g_Indices =
{
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	8, 9, 10, 10, 11, 8,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	20, 21, 22, 22, 23, 20
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

VulkanDeferredApp::VulkanDeferredApp(const std::string_view title, int width, int height)
	: mWinWidth(width), mWinHeight(height), mCurrFrame(0), mFramebufferResized(false)
{
	InitWindow(title, width, height);
}

VulkanDeferredApp::~VulkanDeferredApp()
{

}

void VulkanDeferredApp::Run()
{
	InitVulkan();
	MainLoop();
	Close();
}

void VulkanDeferredApp::InitWindow(const std::string_view title, int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//不创建OpenGL上下文
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	m_pWindow = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_pWindow, this);

	glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int width, int height)
	{
		auto app = (VulkanDeferredApp*)glfwGetWindowUserPointer(pWindow);
		app->SetFramebufferResize(true);
	});
}

void VulkanDeferredApp::InitVulkan()
{
	CreateVulkanInstance();
	SetupDebugCallback();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicDevice();
	CreateSwapChain();
	CreateSwapChainImageView();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateDepthResource();
	CreateRenderPass();
	CreateFrameBuffer();
	CreateSemaphores();

	CreateVertexBuffer();
	CreateIndexBuffer();

	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();

	PrepareOffscreenFrameBuffer();
	OffscreenUniformBuffer();
	CreateDescriptorSetLayout();
	CreateDeferrdPipeline();
	CreateDescriptorPool();
	CreateDescriptorSets();
	BuildCommandBuffers();
	BuildDeferredCommandBuffer();
}

void VulkanDeferredApp::MainLoop()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(m_pDevice);
}

void VulkanDeferredApp::Close()
{

}

void VulkanDeferredApp::DrawFrame()
{
	vkWaitForFences(m_pDevice, 1, &mInFlightFences[mCurrFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(m_pDevice, m_pSwapChain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphores[mCurrFrame], nullptr, &imageIndex);
	//if (result == VK_ERROR_OUT_OF_DATE_KHR)
	//{
	//	//mFramebufferResized = false;
	//	RecreateSwapChain();
	//	return;
	//}
	//else 
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		assert(false);
	}

	//vkResetCommandBuffer(mCommandBuffers[imageIndex], 0);
	//RecordCommandBuffer(mCommandBuffers[imageIndex], imageIndex);

	//UpdateUniformBuffer(imageIndex);
	UpdateOffscreenUniformBuffer();

	//提交指令缓冲
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &m_pOffscreenCmdBuffer;

	VkPipelineStageFlags stageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//与semaphores的索引相对应
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &mImageAvailableSemaphores[mCurrFrame];
	info.pWaitDstStageMask = stageFlags;

	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &m_pOffscreenSemaphore[mCurrFrame];

	vkResetFences(m_pDevice, 1, &mInFlightFences[mCurrFrame]);

	if (vkQueueSubmit(m_pGraphicQueue, 1, &info, mInFlightFences[mCurrFrame]) != VK_SUCCESS)
	{
		std::cerr << "GraphicQueue submit failed" << std::endl;
	}
	vkWaitForFences(m_pDevice, 1, &mInFlightFences[mCurrFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	//scene
	info.pWaitSemaphores = &m_pOffscreenSemaphore[mCurrFrame];
	info.pSignalSemaphores = &mImageFinishedSemaphores[mCurrFrame];

	info.commandBufferCount = 1;
	info.pCommandBuffers = &mCommandBuffers[imageIndex];
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
	presentInfo.pWaitSemaphores = &mImageFinishedSemaphores[mCurrFrame];
	presentInfo.pResults = nullptr;
	VkResult pr_result = vkQueuePresentKHR(m_pGraphicQueue, &presentInfo);
	/*if (pr_result == VK_ERROR_OUT_OF_POOL_MEMORY || pr_result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
	{
		mFramebufferResized = false;
		RecreateSwapChain();
	}
	else */if (pr_result != VK_SUCCESS)
	{
		std::cerr << "QueuePresentKHR failed" << std::endl;
		assert(0);
	}
	//vkQueueWaitIdle(m_pGraphicQueue);
	mCurrFrame = (mCurrFrame + 1) % 2;
}

void VulkanDeferredApp::CreateVulkanInstance()
{
	if (!CheckValidationLayerSupport())
	{
		std::cout << "Validation layers required, but not available" << std::endl;
		return;
	}

	VkApplicationInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pApplicationName = "Hello_Triangle";
	info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
	info.pEngineName = "No_Engine";
	info.engineVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
	info.apiVersion = VK_API_VERSION_1_3;

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> ExtensionsProp(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, ExtensionsProp.data());

	std::vector<const char*> Extensions = GetRequireExtenstion();

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &info;
	createInfo.ppEnabledExtensionNames = Extensions.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();

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

void VulkanDeferredApp::SetupDebugCallback()
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

VkResult VulkanDeferredApp::CreateDebugUtilsMessengerEXT(VkInstance pInstance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* ppCallback)
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

void VulkanDeferredApp::DestroyDebugUtilsMessengerEXT()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_pVKInstance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(m_pVKInstance, m_pDebugUtils, nullptr);
	}
}

void VulkanDeferredApp::CreateSurface()
{
	if (glfwCreateWindowSurface(m_pVKInstance, m_pWindow, nullptr, &m_pSurface) != VK_SUCCESS)
	{
		std::cerr << "VkSurfaceKHR create failed" << std::endl;
	}
}

void VulkanDeferredApp::PickPhysicalDevice()
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

void VulkanDeferredApp::CreateLogicDevice()
{
	QueueFamilyIndex index = FindQueueFamilies(m_pPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::set<int32_t> UniqueQueueFamilise = { index.graphicsFamily, index.presentFamily };

	float priority = 1.f;
	for (int32_t index : UniqueQueueFamilise)
	{
		VkDeviceQueueCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.queueCount = 1;
		createInfo.queueFamilyIndex = index;
		createInfo.pQueuePriorities = &priority;
		QueueCreateInfos.emplace_back(createInfo);
	}

	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &features;
	deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtenstion.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtenstion.data();

	if (vkCreateDevice(m_pPhysicalDevice, &deviceCreateInfo, nullptr, &m_pDevice) != VK_SUCCESS)
	{
		std::cerr << "VKDevice create failed" << std::endl;
	}

	vkGetDeviceQueue(m_pDevice, index.graphicsFamily, 0, &m_pGraphicQueue);
	vkGetDeviceQueue(m_pDevice, index.presentFamily, 0, &m_pPresentQueue);
}

void VulkanDeferredApp::CreateSwapChain()
{
	SwapChainSupportDetails details = QuerySwapChainSupport(m_pPhysicalDevice);
	VkSurfaceFormatKHR format = ChooseSurfaceFormat(details.formates);
	VkPresentModeKHR presentMode = ChooseSwapChainPresentMode(details.presentModes);
	VkExtent2D extent = ChooseSwapExtent(details.capabilities);
	uint32_t imageCoun = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && imageCoun > details.capabilities.maxImageCount)
	{
		imageCoun = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_pSurface;
	createInfo.minImageCount = imageCoun;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.presentMode = presentMode;
	createInfo.clipped = true;

	QueueFamilyIndex index = FindQueueFamilies(m_pPhysicalDevice);
	uint32_t queueFamilyIndices[] = { index.graphicsFamily, index.presentFamily };
	if (index.graphicsFamily != index.presentFamily)
	{
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else
	{
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

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

void VulkanDeferredApp::CreateSwapChainImageView()
{
	mSwapChainImageViews.resize(mSwapChainImages.size());
	for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
	{
		CreateImageView(mSwapChainImages[i], mSwapChainImageViews[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void VulkanDeferredApp::CreateCommandPool()
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

void VulkanDeferredApp::CreateCommandBuffers()
{
	mCommandBuffers.resize(mSwapChainImageViews.size());
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

void VulkanDeferredApp::CreateDepthResource()
{
	VkFormat depthFormat = FindDepthFormat();
	CreateImage(mSwapChainImageExtent.width, mSwapChainImageExtent.height, 1, 1, VK_IMAGE_TYPE_2D,
		depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_pDepthImage, m_pDepthImageMemory);
	CreateImageView(m_pDepthImage, m_pDepthImageView, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	//TransitionImageLayout(m_pDepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void VulkanDeferredApp::CreateRenderPass()
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

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependencyInfo[2] = {};
	dependencyInfo[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencyInfo[0].dstSubpass = 0;
	dependencyInfo[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencyInfo[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencyInfo[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencyInfo[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencyInfo[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencyInfo[1].srcSubpass = 0;
	dependencyInfo[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencyInfo[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencyInfo[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencyInfo[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencyInfo[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencyInfo[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderCreateInfo = {};
	renderCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderCreateInfo.pAttachments = attachments;
	renderCreateInfo.attachmentCount = 2;
	renderCreateInfo.subpassCount = 1;
	renderCreateInfo.pSubpasses = &subpass;
	renderCreateInfo.dependencyCount = 2;
	renderCreateInfo.pDependencies = dependencyInfo;

	if (vkCreateRenderPass(m_pDevice, &renderCreateInfo, nullptr, &m_pRenderPass) != VK_SUCCESS)
	{
		std::cerr << "VkRenderPass create failed" << std::endl;
	}
}

void VulkanDeferredApp::CreateFrameBuffer()
{
	mFrameBuffers.resize(mSwapChainImageViews.size());
	for (size_t i = 0; i < mFrameBuffers.size(); i++)
	{
		VkImageView attachments[] = { mSwapChainImageViews[i], m_pDepthImageView };
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.attachmentCount = 2;
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

void VulkanDeferredApp::CreateSemaphores()
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

void VulkanDeferredApp::CreateVertexBuffer()
{
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

void VulkanDeferredApp::CreateIndexBuffer()
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

void VulkanDeferredApp::CreateTextureImage()
{
	int width, height, comp = 0;
	unsigned char* data = stbi_load("texture/huaji.jpg", &width, &height, &comp, STBI_rgb_alpha);
	VkDeviceSize imageSize = width * height * 4;
	if (!data)
	{
		assert(0);
	}

	mMipLevels = 1;

	VkBuffer pTempBuffer;
	VkDeviceMemory pTempMemory;
	CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, pTempBuffer, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pTempMemory);
	void* pData;
	vkMapMemory(m_pDevice, pTempMemory, 0, imageSize, 0, &pData);
	memcpy(pData, data, imageSize);
	vkUnmapMemory(m_pDevice, pTempMemory);

	stbi_image_free(data);

	CreateImage(width, height, 1, mMipLevels, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_pTextureImage, m_pTextureImageMemory);

	//变换布局
	/*
		这里我们创建的图像对象使用VK_IMAGE_LAYOUT_UNDEFINED布局，所以转换图像布局时应该将VK_IMAGE_LAYOUT_UNDEFINED指定为旧布局.
		要注意的是我们之所以这样设置是因为我们不需要读取复制操作之前的图像内容。
	*/
	TransitionImageLayout(m_pTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipLevels);
	CopyBufferToImage(pTempBuffer, m_pTextureImage, width, height);
	//为了能够在着色器中采样纹理图像数据，我们还要进行1次图像布局变换
	TransitionImageLayout(m_pTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mMipLevels);

	vkDestroyBuffer(m_pDevice, pTempBuffer, nullptr);
	vkFreeMemory(m_pDevice, pTempMemory, nullptr);
}

void VulkanDeferredApp::CreateTextureImageView()
{
	CreateImageView(
		m_pTextureImage, 
		m_pTextureImageView, 
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_ASPECT_COLOR_BIT, 
		mMipLevels);
}

void VulkanDeferredApp::CreateTextureSampler()
{
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.minFilter = VK_FILTER_LINEAR;
	info.magFilter = VK_FILTER_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.anisotropyEnable = VK_TRUE;
	info.maxAnisotropy = 16.f;
	info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.mipLodBias = 0.f;
	info.minLod = 0.f;
	info.maxLod = 0.f;
	info.compareEnable = VK_FALSE;
	info.compareOp = VK_COMPARE_OP_ALWAYS;

	if (vkCreateSampler(m_pDevice, &info, nullptr, &m_pTextureSampler) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanDeferredApp::PrepareOffscreenFrameBuffer()
{
	mOffscreenFrameBuffer.width = mWinWidth;
	mOffscreenFrameBuffer.height = mWinHeight;

	std::vector<VkAttachmentDescription> attachments(4);
	// position
	attachments[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// normal
	attachments[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// albedo
	attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// depth
	attachments[3].format = FindDepthFormat();
	attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference basePassColorAttachmentsRefs[3];
	basePassColorAttachmentsRefs[0].attachment = 0;//position
	basePassColorAttachmentsRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	basePassColorAttachmentsRefs[1].attachment = 1;//normal
	basePassColorAttachmentsRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	basePassColorAttachmentsRefs[2].attachment = 2;//albedo
	basePassColorAttachmentsRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference basePassDepthAttachmentsRef = {};
	basePassDepthAttachmentsRef.attachment = 3;
	basePassDepthAttachmentsRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 3;
	subpassDesc.pColorAttachments = basePassColorAttachmentsRefs;
	subpassDesc.pDepthStencilAttachment = &basePassDepthAttachmentsRef;

	VkSubpassDependency depens[2] = {};
	depens[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	depens[0].dstSubpass = 0;
	depens[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	depens[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	depens[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depens[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	depens[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	depens[1].srcSubpass = 0;
	depens[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	depens[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depens[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	depens[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	depens[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	depens[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderCreateInfo = {};
	renderCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderCreateInfo.pAttachments = attachments.data();
	renderCreateInfo.attachmentCount = attachments.size();
	renderCreateInfo.subpassCount = 1;
	renderCreateInfo.pSubpasses = &subpassDesc;
	renderCreateInfo.dependencyCount = 2;
	renderCreateInfo.pDependencies = depens;

	if (vkCreateRenderPass(m_pDevice, &renderCreateInfo, nullptr, &mOffscreenFrameBuffer.pRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("VkRenderPass create failed!");
	}

	mOffscreenFrameBuffer.attachments.resize(4);
	for (int i = 0; i < 4; i++)
	{
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		VkImageAspectFlags flags = VK_IMAGE_ASPECT_COLOR_BIT;
		if (i == 2)
		{
			format = VK_FORMAT_R8G8B8A8_UNORM;
		}
		else if (i == 3)
		{
			format = attachments[3].format;
			flags = VK_IMAGE_ASPECT_DEPTH_BIT;
			usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		CreateImage(mOffscreenFrameBuffer.width, mOffscreenFrameBuffer.height, 1, 1,
			VK_IMAGE_TYPE_2D, format, VK_IMAGE_TILING_OPTIMAL,
			usage | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			mOffscreenFrameBuffer.attachments[i].pImage, mOffscreenFrameBuffer.attachments[i].pMemory);
		CreateImageView(mOffscreenFrameBuffer.attachments[i].pImage, mOffscreenFrameBuffer.attachments[i].pImageView,
			format, flags, 1);
	}

	VkImageView pViews[] = {
		mOffscreenFrameBuffer.attachments[0].pImageView,
		mOffscreenFrameBuffer.attachments[1].pImageView,
		mOffscreenFrameBuffer.attachments[2].pImageView,
		mOffscreenFrameBuffer.attachments[3].pImageView
	};
	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.attachmentCount = 4;
	createInfo.pAttachments = pViews;
	createInfo.width = mOffscreenFrameBuffer.width;
	createInfo.height = mOffscreenFrameBuffer.height;
	createInfo.renderPass = mOffscreenFrameBuffer.pRenderPass;
	createInfo.layers = 1;

	if (vkCreateFramebuffer(m_pDevice, &createInfo, nullptr, &mOffscreenFrameBuffer.pFrameBuffer) != VK_SUCCESS)
	{
		std::cerr << "VkFramebuffer create failed" << std::endl;
		return;
	}

	// Create sampler to sample from the color attachments
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.minFilter = VK_FILTER_NEAREST;
	info.magFilter = VK_FILTER_NEAREST;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.maxAnisotropy = 1.f;
	info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.mipLodBias = 0.f;
	info.minLod = 0.f;
	info.maxLod = 1.f;

	if (vkCreateSampler(m_pDevice, &info, nullptr, &pColorSampler) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanDeferredApp::OffscreenUniformBuffer()
{
	//offscreen ubo
	VkDeviceSize size = sizeof(UniformBufferObj);
	CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, mOffscreenUbo.pBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mOffscreenUbo.pMem);
	// Deferred ubo
	CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, mCompositionUbo.pBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mCompositionUbo.pMem);

	UpdateOffscreenUniformBuffer();
}

void VulkanDeferredApp::UpdateOffscreenUniformBuffer()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currTime - startTime).count();

	UniformBufferObj ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.f), glm::radians(10.f) * time, { 1.f, 1.f, 0.f });
	ubo.view = glm::lookAt(glm::vec3(0.f, 0.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	ubo.proj = glm::perspective(glm::radians(45.f), (float)mSwapChainImageExtent.width / mSwapChainImageExtent.height, 0.1f, 1000.f);
	//ubo.proj[1][1] *= -1;

	void* pData;
	vkMapMemory(m_pDevice, mOffscreenUbo.pMem, 0, sizeof(ubo), 0, &pData);
	memcpy(pData, &ubo, sizeof(ubo));
	vkUnmapMemory(m_pDevice, mOffscreenUbo.pMem);
}

void VulkanDeferredApp::CreateDescriptorSetLayout()
{
	//deferred shading
	std::vector<VkDescriptorSetLayoutBinding> deferredBinding(5);
	//vertex shader uniform buffer
	deferredBinding[0].binding = 0;
	deferredBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	deferredBinding[0].descriptorCount = 1;
	deferredBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//position texture target
	deferredBinding[1].binding = 1;
	deferredBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredBinding[1].descriptorCount = 1;
	deferredBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	//normal texture target
	deferredBinding[2].binding = 2;
	deferredBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredBinding[2].descriptorCount = 1;
	deferredBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	//albedo texture target
	deferredBinding[3].binding = 3;
	deferredBinding[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredBinding[3].descriptorCount = 1;
	deferredBinding[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	//texture sampler
	deferredBinding[4].binding = 4;
	deferredBinding[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	deferredBinding[4].descriptorCount = 1;
	deferredBinding[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkDescriptorSetLayoutCreateInfo deferredLayoutInfo = {};
	deferredLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	deferredLayoutInfo.bindingCount = 5;
	deferredLayoutInfo.pBindings = deferredBinding.data();

	if (vkCreateDescriptorSetLayout(m_pDevice, &deferredLayoutInfo, nullptr, &m_pDescriptorSetLayout) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanDeferredApp::CreateDeferrdPipeline()
{
	std::vector<char> vertexShaderCode = ReadFile("shader/deferred_composition.vert.spv");
	std::vector<char> fragmentShaderCode = ReadFile("shader/deferred_composition.frag.spv");
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
	colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;//0xf;
	colorBlendState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendState;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthInfo.depthBoundsTestEnable = VK_FALSE;
	depthInfo.stencilTestEnable = VK_FALSE;

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
	graphicsPipelineCreateInfo.pDepthStencilState = &depthInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateInfo;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.renderPass = m_pRenderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.layout = m_pPipelineLayout;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	// Empty vertex input state, vertices are generated by the vertex shader
	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
	vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateInfo;

	if (vkCreateGraphicsPipelines(m_pDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_pPipeline) != VK_SUCCESS)
	{
		std::cerr << "VkPipeline create failed" << std::endl;
		return;
	}
	vkDestroyShaderModule(m_pDevice, pVertexShaderModule, nullptr);
	vkDestroyShaderModule(m_pDevice, pFragmentShaderModule, nullptr);

	auto descriptions = VertexData::GetVertexInputAttributeDescriptions();
	vertexInputStateInfo.vertexAttributeDescriptionCount = (uint32_t)descriptions.size();
	vertexInputStateInfo.pVertexAttributeDescriptions = descriptions.data();
	vertexInputStateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateInfo.pVertexBindingDescriptions = &VertexData::GetBindingDescription();

	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateInfo;
	graphicsPipelineCreateInfo.renderPass = mOffscreenFrameBuffer.pRenderPass;

	vertexShaderCode = ReadFile("shader/deferred.vert.spv");
	fragmentShaderCode = ReadFile("shader/deferred.frag.spv");
	pVertexShaderModule = CreateShaderModule(vertexShaderCode);
	pFragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	vertexShaderCreateInfo.module = pVertexShaderModule;
	fragmentShaderCreateInfo.module = pFragmentShaderModule;
	VkPipelineShaderStageCreateInfo offscreenShaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = offscreenShaderStages;

	VkPipelineColorBlendAttachmentState offscreenColorBlendStates[3] = {};
	offscreenColorBlendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;//0xf;
	offscreenColorBlendStates[0].blendEnable = VK_FALSE;
	offscreenColorBlendStates[1].colorWriteMask = 0xf;
	offscreenColorBlendStates[1].blendEnable = VK_FALSE;
	offscreenColorBlendStates[2].colorWriteMask = 0xf;
	offscreenColorBlendStates[2].blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo offscreenColorBlendStateCreateInfo = {};
	offscreenColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	offscreenColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	offscreenColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	offscreenColorBlendStateCreateInfo.attachmentCount = 3;
	offscreenColorBlendStateCreateInfo.pAttachments = offscreenColorBlendStates;

	graphicsPipelineCreateInfo.pColorBlendState = &offscreenColorBlendStateCreateInfo;

	if (vkCreateGraphicsPipelines(m_pDevice, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_pOffscerrnPipeline) != VK_SUCCESS)
	{
		std::cerr << "VkPipeline create failed" << std::endl;
		return;
	}

	vkDestroyShaderModule(m_pDevice, pVertexShaderModule, nullptr);
	vkDestroyShaderModule(m_pDevice, pFragmentShaderModule, nullptr);
}

void VulkanDeferredApp::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize[2];
	poolSize[0].descriptorCount = (uint32_t)mSwapChainImages.size() + 8;
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[1].descriptorCount = (uint32_t)mSwapChainImages.size() + 8;
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = 2;
	info.pPoolSizes = poolSize;
	info.maxSets = (uint32_t)mSwapChainImages.size();

	if (vkCreateDescriptorPool(m_pDevice, &info, nullptr, &m_pDescriptorPool) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanDeferredApp::CreateDescriptorSets()
{
	// Image descriptors for the offscreen color attachments
	VkDescriptorImageInfo texPosition{};
	texPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texPosition.imageView = mOffscreenFrameBuffer.attachments[0].pImageView;
	texPosition.sampler = pColorSampler;
	VkDescriptorImageInfo texNormal{};
	texNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texNormal.imageView = mOffscreenFrameBuffer.attachments[1].pImageView;
	texNormal.sampler = pColorSampler;
	VkDescriptorImageInfo texAlbedo{};
	texAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texAlbedo.imageView = mOffscreenFrameBuffer.attachments[2].pImageView;
	texAlbedo.sampler = pColorSampler;

	VkDescriptorSetAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorSetCount = 1;
	info.pSetLayouts = &m_pDescriptorSetLayout;
	info.descriptorPool = m_pDescriptorPool;
	if (vkAllocateDescriptorSets(m_pDevice, &info, &m_pDeferredSet) != VK_SUCCESS)
	{
		assert(0);
	}
	std::vector<VkWriteDescriptorSet> writeDescSets(3);
	// Binding 1 : Position texture target
	VkWriteDescriptorSet& writePosition = writeDescSets[0];
	writePosition.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writePosition.dstSet = m_pDeferredSet;
	writePosition.dstBinding = 1;
	writePosition.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writePosition.descriptorCount = 1;
	writePosition.pImageInfo = &texPosition;
	// Binding 2 : Normals texture target
	VkWriteDescriptorSet& writeNormal = writeDescSets[1];
	writeNormal.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeNormal.dstSet = m_pDeferredSet;
	writeNormal.dstBinding = 2;
	writeNormal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeNormal.descriptorCount = 1;
	writeNormal.pImageInfo = &texNormal;
	// Binding 3 : Albedo texture target
	VkWriteDescriptorSet& writeAlbedo = writeDescSets[2];
	writeAlbedo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeAlbedo.dstSet = m_pDeferredSet;
	writeAlbedo.dstBinding = 3;
	writeAlbedo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeAlbedo.descriptorCount = 1;
	writeAlbedo.pImageInfo = &texAlbedo;
	vkUpdateDescriptorSets(m_pDevice, 3, writeDescSets.data(), 0, nullptr);

	//模型的描述符集
	/*std::vector<VkDescriptorSetLayout> layouts(mSwapChainImages.size(), m_pDescriptorSetLayout);
	VkDescriptorSetAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorSetCount = (uint32_t)layouts.size();
	info.pSetLayouts = layouts.data();
	info.descriptorPool = m_pDescriptorPool;

	mDescriptorSets.resize(mSwapChainImages.size());*/
	if (vkAllocateDescriptorSets(m_pDevice, &info, &m_pModelSet) != VK_SUCCESS)
	{
		assert(0);
	}

	VkDescriptorBufferInfo modelBufferinfo = {};
	modelBufferinfo.buffer = mOffscreenUbo.pBuffer;
	modelBufferinfo.offset = 0;
	modelBufferinfo.range = sizeof(UniformBufferObj);
	VkWriteDescriptorSet writeSet = {};
	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.dstSet = m_pModelSet;
	writeSet.dstBinding = 0;
	writeSet.dstArrayElement = 0;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeSet.descriptorCount = 1;
	writeSet.pBufferInfo = &modelBufferinfo;

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_pTextureImageView;
	imageInfo.sampler = m_pTextureSampler;
	VkWriteDescriptorSet imageWriteSet = {};
	imageWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	imageWriteSet.dstSet = m_pModelSet;
	imageWriteSet.dstBinding = 4;
	imageWriteSet.dstArrayElement = 0;
	imageWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWriteSet.descriptorCount = 1;
	imageWriteSet.pImageInfo = &imageInfo;

	VkWriteDescriptorSet writes[] = { writeSet, imageWriteSet };
	vkUpdateDescriptorSets(m_pDevice, 2, writes, 0, nullptr);
}

void VulkanDeferredApp::BuildCommandBuffers()
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	info.pInheritanceInfo = nullptr;

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = m_pRenderPass;
	VkRect2D renderArea = {};
	renderArea.extent = mSwapChainImageExtent;
	renderArea.offset = { 0, 0 };
	beginInfo.renderArea = renderArea;
	VkClearValue clearValue[2] = {};
	clearValue[0].color = { 0.f, 0.f, 0.f, 1.f };
	clearValue[1].depthStencil = { 1.f, 0 };
	beginInfo.clearValueCount = 2;
	beginInfo.pClearValues = clearValue;

	for (size_t i = 0; i < mCommandBuffers.size(); i++)
	{
		if (vkBeginCommandBuffer(mCommandBuffers[i], &info) != VK_SUCCESS)
		{
			std::cerr << "vkBeginCommandBuffer failed" << std::endl;
			return;
		}

		beginInfo.framebuffer = mFrameBuffers[i];

		vkCmdBeginRenderPass(mCommandBuffers[i], &beginInfo, VK_SUBPASS_CONTENTS_INLINE); //VK_SUBPASS_CONTENTS_INLINE : 所有要执行的指令都在主要指令缓冲中，没有辅助指令缓冲需要执行

		vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)mSwapChainImageExtent.width;
		viewport.height = (float)mSwapChainImageExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(mCommandBuffers[i], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = mSwapChainImageExtent;
		vkCmdSetScissor(mCommandBuffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDeferredSet, 0, nullptr);

		vkCmdDraw(mCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(mCommandBuffers[i]);

		if (vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS)
		{
			std::cerr << "vkEndCommandBuffer failed" << std::endl;
		}
	}
}

void VulkanDeferredApp::BuildDeferredCommandBuffer()
{
	if (m_pOffscreenCmdBuffer == VK_NULL_HANDLE)
	{
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = m_pCommandPool;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(m_pDevice, &info, &m_pOffscreenCmdBuffer) != VK_SUCCESS)
		{
			std::cerr << "VkCommandBuffer create failed" << std::endl;
			return;
		}
	}

	if (m_pOffscreenSemaphore.size() == 0)
	{
		m_pOffscreenSemaphore.resize(2);
		VkSemaphoreCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(m_pDevice, &createInfo, nullptr, &m_pOffscreenSemaphore[0]) != VK_SUCCESS
			|| vkCreateSemaphore(m_pDevice, &createInfo, nullptr, &m_pOffscreenSemaphore[1]) != VK_SUCCESS)
		{
			std::cerr << "VkSemaphore create failed" << std::endl;
			return;
		}
	}

	VkClearValue clearVals[4] = {};
	clearVals[0].color = { 0.f, 0.f, 0.f, 0.f };
	clearVals[1].color = { 0.f, 0.f, 0.f, 0.f };
	clearVals[2].color = { 0.f, 0.f, 0.f, 0.f };
	clearVals[3].depthStencil = { 1.f, 0 };

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = mOffscreenFrameBuffer.pRenderPass;
	VkRect2D renderArea = {};
	renderArea.extent.width = mOffscreenFrameBuffer.width;
	renderArea.extent.height = mOffscreenFrameBuffer.height;
	renderArea.offset = { 0, 0 };
	beginInfo.renderArea = renderArea;
	beginInfo.clearValueCount = 4;
	beginInfo.pClearValues = clearVals;
	beginInfo.framebuffer = mOffscreenFrameBuffer.pFrameBuffer;

	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	info.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(m_pOffscreenCmdBuffer, &info) != VK_SUCCESS)
	{
		std::cerr << "vkBeginCommandBuffer failed" << std::endl;
		return;
	}

	vkCmdBeginRenderPass(m_pOffscreenCmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)mOffscreenFrameBuffer.width;
	viewport.height = (float)mOffscreenFrameBuffer.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_pOffscreenCmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = { mOffscreenFrameBuffer.width, mOffscreenFrameBuffer.height };
	vkCmdSetScissor(m_pOffscreenCmdBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(m_pOffscreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pOffscerrnPipeline);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_pOffscreenCmdBuffer, 0, 1, &m_pVertexBuffer, &offset);
	vkCmdBindIndexBuffer(m_pOffscreenCmdBuffer, m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(m_pOffscreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pModelSet, 0, nullptr);
	vkCmdDrawIndexed(m_pOffscreenCmdBuffer, g_Indices.size(), 1, 0, 0, 0);

	vkCmdEndRenderPass(m_pOffscreenCmdBuffer);

	if (vkEndCommandBuffer(m_pOffscreenCmdBuffer) != VK_SUCCESS)
	{
		std::cerr << "vkEndCommandBuffer failed" << std::endl;
	}
}

bool VulkanDeferredApp::CheckValidationLayerSupport() const
{
	uint32_t count = 0;
	vkEnumerateInstanceLayerProperties(&count, nullptr);
	std::vector<VkLayerProperties> Layers(count);
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

std::vector<const char*> VulkanDeferredApp::GetRequireExtenstion() const
{
	uint32_t glfwExtenstionCount = 0;
	const char** glfwExtenstions = glfwGetRequiredInstanceExtensions(&glfwExtenstionCount);

	std::vector<const char*> Extenstions(glfwExtenstions, glfwExtenstions + glfwExtenstionCount);

	//if debug
	Extenstions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return Extenstions;
}

bool VulkanDeferredApp::IsDeviceSuitable(VkPhysicalDevice pDevice) const
{
	QueueFamilyIndex index = FindQueueFamilies(pDevice);

	bool extenstionSupport = CheckDeviceExtenstionSupport(pDevice);

	VkPhysicalDeviceFeatures featuresSupport;
	vkGetPhysicalDeviceFeatures(pDevice, &featuresSupport);

	bool swapChainAdequate = false;
	if (extenstionSupport)
	{
		SwapChainSupportDetails details = QuerySwapChainSupport(pDevice);
		swapChainAdequate = !details.formates.empty() && !details.presentModes.empty();
	}
	return index.IsComplete() && extenstionSupport && swapChainAdequate && featuresSupport.samplerAnisotropy;
}

QueueFamilyIndex VulkanDeferredApp::FindQueueFamilies(VkPhysicalDevice pDevice) const
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

bool VulkanDeferredApp::CheckDeviceExtenstionSupport(VkPhysicalDevice pDevice) const
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

SwapChainSupportDetails VulkanDeferredApp::QuerySwapChainSupport(VkPhysicalDevice pDevice) const
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

VkSurfaceFormatKHR VulkanDeferredApp::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
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

VkPresentModeKHR VulkanDeferredApp::ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availabePresentModes) const
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

VkExtent2D VulkanDeferredApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
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
		extent2D.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent2D.width));
		extent2D.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent2D.height));

		return extent2D;
	}
}

void VulkanDeferredApp::CreateImage(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags propertie, VkImage& pImage, VkDeviceMemory& pMemory)
{
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = imageType;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(m_pDevice, &createInfo, nullptr, &pImage) != VK_SUCCESS)
	{
		assert(0);
	}

	VkMemoryRequirements memoryRequirement;
	vkGetImageMemoryRequirements(m_pDevice, pImage, &memoryRequirement);

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirement.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirement.memoryTypeBits, propertie);

	if (vkAllocateMemory(m_pDevice, &allocateInfo, nullptr, &pMemory) != VK_SUCCESS)
	{
		assert(0);
	}

	vkBindImageMemory(m_pDevice, pImage, pMemory, 0);
}

void VulkanDeferredApp::CreateImageView(VkImage pImage, VkImageView& pImageView, VkFormat format, VkImageAspectFlags aspectMask, uint32_t mipLevels)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = pImage;
	createInfo.format = format;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(m_pDevice, &createInfo, nullptr, &pImageView) != VK_SUCCESS)
	{
		assert(0);
	}
}

void VulkanDeferredApp::CreateBuffer(VkBufferUsageFlags usage, VkDeviceSize size, VkBuffer& pBuffer, VkMemoryPropertyFlags Property, VkDeviceMemory& pMemory)
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

uint32_t VulkanDeferredApp::FindMemoryType(uint32_t fliter, VkMemoryPropertyFlags properties)
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

VkFormat VulkanDeferredApp::FindDepthFormat()
{
	return FindSupportFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanDeferredApp::FindSupportFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (auto& format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_pPhysicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	assert(0);
	return VK_FORMAT_UNDEFINED;
}

void VulkanDeferredApp::TransitionImageLayout(VkImage pImage, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer pCommandBuffer = BeginSingleTimeCommands();
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = pImage;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;//不需要传输队列所有权，必须设置为VK_QUEUE_FAMILY_IGNORED
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;//不需要传输队列所有权，必须设置为VK_QUEUE_FAMILY_IGNORED
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (HasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}

		VkPipelineStageFlags srcStageMask;
		VkPipelineStageFlags dstStageMask;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			assert(0);
		}

		vkCmdPipelineBarrier(pCommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}
	EndSingleTimeCommands(pCommandBuffer);
}

bool VulkanDeferredApp::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanDeferredApp::CopyBufferToImage(VkBuffer pBuffer, VkImage pImage, uint32_t width, uint32_t height)
{
	VkCommandBuffer pCommanndBuffer = BeginSingleTimeCommands();
	{
		VkBufferImageCopy regions = {};
		regions.bufferOffset = 0;
		regions.bufferRowLength = 0;//bufferRowLength和bufferImageHeight成员变量用于指定数据在内存中的存放方式。通过这两个成员变量我们可以对每行图像数据使用额外的空间进行对齐。将这两个成员变量的值都设置为0，数据将会在内存中被紧凑存放。
		regions.bufferImageHeight = 0;
		regions.imageExtent.width = width;
		regions.imageExtent.height = height;
		regions.imageExtent.depth = 1;
		regions.imageOffset = { 0, 0, 0 };
		regions.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions.imageSubresource.layerCount = 1;
		regions.imageSubresource.mipLevel = 0;
		regions.imageSubresource.baseArrayLayer = 0;
		vkCmdCopyBufferToImage(pCommanndBuffer, pBuffer, pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions);
	}
	EndSingleTimeCommands(pCommanndBuffer);
}

void VulkanDeferredApp::CopyBuffer(VkBuffer pSrcBuffer, VkBuffer pDstBuffer, VkDeviceSize size)
{
	VkCommandBuffer pCommandBuffer = BeginSingleTimeCommands();
	{
		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = size;

		vkCmdCopyBuffer(pCommandBuffer, pSrcBuffer, pDstBuffer, 1, &bufferCopy);
	}
	EndSingleTimeCommands(pCommandBuffer);
}

VkShaderModule VulkanDeferredApp::CreateShaderModule(const std::vector<char>& shaderCode) const
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

VkCommandBuffer VulkanDeferredApp::BeginSingleTimeCommands()
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

	return pCommandBuffer;
}

void VulkanDeferredApp::EndSingleTimeCommands(VkCommandBuffer pCommandBuffer)
{
	vkEndCommandBuffer(pCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &pCommandBuffer;
	vkQueueSubmit(m_pGraphicQueue, 1, &submitInfo, nullptr);
	vkQueueWaitIdle(m_pGraphicQueue);

	vkFreeCommandBuffers(m_pDevice, m_pCommandPool, 1, &pCommandBuffer);
}

