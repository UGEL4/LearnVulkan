#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string_view>
#include <string>
#include <vector>
#include <iostream>

struct QueueFamilyIndex
{
	int32_t graphicsFamily;
	int32_t presentFamily;

	QueueFamilyIndex() : graphicsFamily(-1) {}

	bool IsComplete() const
	{
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formates;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanApp
{
public:
	VulkanApp(const std::string_view title, int width, int height);
	~VulkanApp();

public:
	void Run();
private:
	void InitWindow(const std::string_view title, int width, int height);
	void InitVulkan();
	void MainLoop();
	void Close();

	void CreateVulkanInstance();
	bool CheckValidationLayerSupport() const;
	std::vector<const char*> GetRequireExtenstion() const;
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice pDevice) const;
	QueueFamilyIndex FindQueueFamilies(VkPhysicalDevice pDevice) const;
	void CreateLogicDevice();

	void SetupDebugCallback();
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance pInstance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* ppCallback);
	void DestroyDebugUtilsMessengerEXT();

	void CreateSurface();
	bool CheckDeviceExtenstionSupport(VkPhysicalDevice pDevice) const;

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice pDevice) const;
	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availabePresentModes) const;
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
	void CreateSwapChain();
	void CreateSwapChainImageView();

	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode) const;
private:
	int mWinWidth;
	int mWinHeight;
	GLFWwindow* m_pWindow;
	VkInstance m_pVKInstance;
	VkDebugUtilsMessengerEXT m_pDebugUtils;
	VkPhysicalDevice m_pPhysicalDevice = VK_NULL_HANDLE;//自动销毁
	VkDevice m_pDevice;
	VkQueue m_pGraphicQueue;//自动销毁
	VkSurfaceKHR m_pSurface;
	VkQueue m_pPresentQueue;

	VkSwapchainKHR m_pSwapChain;
	std::vector<VkImage> mSwapChainImages;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainImageExtent;
	std::vector<VkImageView> mSwapChainImageViews;

	VkShaderModule m_pVertexShaderModule;
	VkShaderModule m_pFragmentShaderModule;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
	void*                                            pUserData
)
{
	std::cerr << "\tValidationLayer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}