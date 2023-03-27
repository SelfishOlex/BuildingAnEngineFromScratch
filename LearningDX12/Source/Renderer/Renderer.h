
#pragma once

#include <optional>
#include <vector>
#include <Vertex.h>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Renderer
{
public:
    void Init(GLFWwindow* window);

    void Cleanup();

    void DrawFrame();

    void OnExitMainLoop();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    void InitVulkan();
    void InitImGui();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamiliesWithSurfaces(VkPhysicalDevice device);

    void cleanupSwapChain();
    void RecreateSwapChain();
    void createSwapChain();
    void createImageViews();
    void createFramebuffers();

    void createVertexBuffer();
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    bool enableValidationLayers = true;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const std::vector<Vertex> vertices =
    {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}}
    };

    GLFWwindow* m_window;
    VkPhysicalDevice m_physicalDevice;
    VkInstance instance;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages; // no cleanup needed    
    std::vector<VkImageView> swapChainImageViews;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;

    bool framebufferResized = false;

    uint32_t queueRenderFamily = 0;

    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Command buffer and synchronization objects per frame in the swap chain
    struct FrameObjects
    {
        VkCommandBuffer commandBuffer;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    };

    std::vector<FrameObjects> frameObjects;
    uint32_t currentFrame = 0;

    // synchronization objects
    /*VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;*/

    // Swap chain related data
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<char> ReadFile(const char* filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
