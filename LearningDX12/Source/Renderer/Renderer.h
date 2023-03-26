
#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
};

class Renderer
{
public:
    void InitVulkan();
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    void MainLoop();

    void Cleanup();

private:
    bool enableValidationLayers = true;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstance instance;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
};
