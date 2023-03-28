
#pragma once

#include <vulkan/vulkan_core.h>

class Renderer;

class GpuImage
{
public:
    void CreateFromTextureFile(Renderer& renderer, const char* texturePath);
    void Release(Renderer& renderer);

    VkImage         m_image = VK_NULL_HANDLE;
    VkDeviceMemory  m_deviceMemory = VK_NULL_HANDLE;
    VkImageView     m_view = VK_NULL_HANDLE;

private:
    VkImageView CreateImageView(Renderer& renderer, VkFormat format, VkImageAspectFlags aspectFlags);
};

