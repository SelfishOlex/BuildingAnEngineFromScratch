
#include "GpuImage.h"

#include <cassert>
#include <cstring>
#include <Renderer.h>
#include <stb_image/stb_image.h>


void GpuImage::CreateFromTextureFile(Renderer& renderer, const char* texturePath)
{
    // texture -> staging buffer -> copy -> local GPU buffer

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    assert(pixels);
    const VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    renderer.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(renderer.m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(renderer.m_device, stagingBufferMemory);

    stbi_image_free(pixels);

    // create a texture image in GPU memory

    renderer.CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT /*as a destination for copy from staging buffer*/ | VK_IMAGE_USAGE_SAMPLED_BIT /*as a sampler for shaders*/,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /*Bind to local GPU buffer*/, m_image, m_deviceMemory);

    renderer.TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    renderer.CopyBufferToImage(stagingBuffer, m_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    renderer.TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(renderer.m_device, stagingBuffer, nullptr);
    vkFreeMemory(renderer.m_device, stagingBufferMemory, nullptr);

    m_view = CreateImageView(renderer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void GpuImage::Release(Renderer& renderer)
{
    vkDestroyImageView(renderer.m_device, m_view, nullptr);
    vkDestroyImage(renderer.m_device, m_image, nullptr);
    vkFreeMemory(renderer.m_device, m_deviceMemory, nullptr);
}

VkImageView GpuImage::CreateImageView(Renderer& renderer, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    assert(vkCreateImageView(renderer.m_device, &viewInfo, nullptr, &imageView) == VK_SUCCESS);

    return imageView;
}
