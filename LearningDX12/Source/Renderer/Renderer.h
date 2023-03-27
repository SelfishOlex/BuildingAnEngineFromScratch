
#pragma once

#include <stb_image/stb_image.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <optional>
#include <vector>
#include <Vertex.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
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

/*
 * Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
 * A vec2 must be aligned by 2N (= 8 bytes)
 * A vec3 or vec4 must be aligned by 4N (= 16 bytes)
 * A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
 * A mat4 matrix must have the same alignment as a vec4.
 */
struct UniformBufferObject
{
    // Example, if a vec2 is added at the beginning
    // glm::vec2 foo;
    // alignas(16) glm::mat4 model;

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

// Command buffer and synchronization objects per frame in the swap chain
struct FrameObjects
{
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    // Uniform attribute/data stuff
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBuffersMemory;
    void* uniformBuffersMapped;

    VkDescriptorSet descriptorSet;

    void CleanUp(VkDevice device);
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
    void createTextureImageView();
    VkImageView createImageView(VkImage image, VkFormat format);
    void createTextureSampler();
    void InitVulkan();
    void InitImGui();

    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createRenderPass();
    void createGraphicsPipeline();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void createTextureImage();
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);


    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamiliesWithSurfaces(VkPhysicalDevice device);

    void createDescriptorPool();
    void createDescriptorSets();
    void cleanupSwapChain();
    void RecreateSwapChain();
    void createSwapChain();
    void createImageViews();
    void createFramebuffers();

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createVertexBuffer();
    void createIndexBuffer();
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createDescriptorSetLayout();

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkImage  textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    bool enableValidationLayers = true;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2,
        2, 3, 0
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
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;

    bool framebufferResized = false;

    uint32_t queueRenderFamily = 0;

    const int MAX_FRAMES_IN_FLIGHT = 2;

    void createUniformBuffers();

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
