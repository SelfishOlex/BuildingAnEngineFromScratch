
#pragma once

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_core.h>

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoordinates;

    /*
     * A vertex binding describes at which rate to load data from memory throughout the vertices.
     * It specifies the number of bytes between data entries and whether to move to the next data entry
     * after each vertex or after each instance.
     */
    static VkVertexInputBindingDescription getBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

