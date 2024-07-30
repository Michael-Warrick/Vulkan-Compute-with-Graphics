#pragma once

#include <vulkan/vulkan.hpp>

class Utilities
{
public:
    static vk::CommandBuffer beginSingleTimeCommands(vk::Device logicalDevice, vk::CommandPool commandPool);
    static void endSingleTimeCommands(vk::Device logicalDevice, vk::Queue queue, vk::CommandBuffer commandBuffer, vk::CommandPool commandPool);

    static void createBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer &buffer, vk::DeviceMemory &bufferMemory);
    static void copyBuffer(vk::Device logicalDevice, vk::Queue queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool commandPool);

    static uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};