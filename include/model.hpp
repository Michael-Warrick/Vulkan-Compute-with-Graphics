#pragma once
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "stb_image/stb_image.h"
#include "tiny_obj_loader/tiny_obj_loader.h"

#include <iostream>
#include <unordered_map>

#include "utilities.hpp"

class Model
{
public:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 textureCoordinates;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription()
                                                                       .setBinding(0)
                                                                       .setStride(sizeof(Vertex))
                                                                       .setInputRate(vk::VertexInputRate::eVertex);

            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0]
                .setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(offsetof(Vertex, position));

            attributeDescriptions[1]
                .setBinding(0)
                .setLocation(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(offsetof(Vertex, color));

            attributeDescriptions[2]
                .setBinding(0)
                .setLocation(2)
                .setFormat(vk::Format::eR32G32Sfloat)
                .setOffset(offsetof(Vertex, textureCoordinates));

            return attributeDescriptions;
        }

        bool operator==(const Vertex &other) const
        {
            return position == other.position && color == other.color && textureCoordinates == other.textureCoordinates;
        }

        friend struct std::hash<Vertex>;
    };

    struct VertexHasher
    {
        size_t operator()(Vertex const &vertex) const
        {
            return ((std::hash<glm::vec3>()(vertex.position) ^
                     (std::hash<glm::vec3>()(vertex.color) << 1)) >>
                    1) ^
                   (std::hash<glm::vec2>()(vertex.textureCoordinates) << 1);
        }
    };

    void Load(const char *modelPath, vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::Queue queue, vk::CommandPool commandPool);
    void Draw(vk::CommandBuffer commandBuffer);
    void Destroy(vk::Device logicalDevice);

private:
    void loadModel(const char *path);

    void createVertexBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::Queue queue, vk::CommandPool commandPool);
    void createIndexBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::Queue queue, vk::CommandPool commandPool);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;
};
