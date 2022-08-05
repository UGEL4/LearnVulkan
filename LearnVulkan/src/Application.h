#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string_view>
#include <string>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>

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

struct VertexData
{
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDesc = {};
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(VertexData);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}

	static std::vector<VkVertexInputAttributeDescription> GetVertexInputAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> descs(2);
		VkVertexInputAttributeDescription attributeDesc = {};
		descs[0].location = 0;
		descs[0].binding = 0;
		descs[0].offset = offsetof(VertexData, position);
		descs[0].format = VK_FORMAT_R32G32_SFLOAT;
		//descs.emplace_back(attributeDesc);

		descs[1].location = 1;
		descs[1].binding = 0;
		descs[1].offset = offsetof(VertexData, color);
		descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		//descs.emplace_back(attributeDesc);

		return descs;
	}
};

struct UniformBufferObj
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class VulkanApp
{
public:
	VulkanApp(const std::string_view title, int width, int height);
	~VulkanApp();

public:
	void Run();
	void SetFramebufferResize(bool state) { mFramebufferResized = state; }
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
	void RecreateSwapChain();
	void CleanupSwapChain();
	void CreateSwapChainImageView();

	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode) const;
	void CreateRenderPass();

	void CreateFrameBuffer();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateSemaphores();

	void DrawFrame();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateBuffer(VkBufferUsageFlags usage, VkDeviceSize size, VkBuffer& pBuffer, VkMemoryPropertyFlags Property, VkDeviceMemory& pMemory);
	uint32_t FindMemoryType(uint32_t fliter, VkMemoryPropertyFlags properties);
	void CopyBuffer(VkBuffer pSrcBuffer, VkBuffer pDstBuffer, VkDeviceSize size);

	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t imageIndex);
	void CreateDescriptorPool();
	void CreateDescriptorSets();

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

	VkRenderPass m_pRenderPass;
	VkDescriptorSetLayout m_pDescriptorSetLayout;
	VkPipelineLayout m_pPipelineLayout;
	VkPipeline m_pPipeline;

	std::vector<VkFramebuffer> mFrameBuffers;

	VkCommandPool m_pCommandPool;
	std::vector<VkCommandBuffer> mCommandBuffers;

	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mImageFinishedSemaphores;
	std::vector<VkFence> mInFlightFences;
	int mCurrFrame;
	bool mFramebufferResized;

	VkBuffer m_pVertexBuffer;
	VkDeviceMemory m_pVertexBufferMemory;
	VkBuffer m_pIndexBuffer;
	VkDeviceMemory m_pIndexBufferMemory;

	std::vector<VkBuffer> mUniformBuffers;
	std::vector<VkDeviceMemory> mUniformBuffersMemory;

	VkDescriptorPool m_pDescriptorPool;
	std::vector<VkDescriptorSet> mDescriptorSets;//描述符集对象会在描述符池对象清除时自动被清除
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