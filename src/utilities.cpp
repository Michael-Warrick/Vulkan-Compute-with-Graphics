#include "utilities.hpp"

vk::CommandBuffer Utilities::beginSingleTimeCommands(vk::Device logicalDevice, vk::CommandPool commandPool)
{
    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
                                                     .setLevel(vk::CommandBufferLevel::ePrimary)
                                                     .setCommandPool(commandPool)
                                                     .setCommandBufferCount(1);

    vk::CommandBuffer commandBuffer;
    vk::Result result = logicalDevice.allocateCommandBuffers(&allocateInfo, &commandBuffer);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffers! Error Code: " + vk::to_string(result));
    }

    vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo()
                                                            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    result = commandBuffer.begin(&commandBufferBeginInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to begin command buffer! Error Code: " + vk::to_string(result));
    }

    return commandBuffer;
}

void Utilities::endSingleTimeCommands(vk::Device logicalDevice, vk::Queue queue, vk::CommandBuffer commandBuffer, vk::CommandPool commandPool)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setCommandBufferCount(1)
                                    .setPCommandBuffers(&commandBuffer);

    vk::Result result = queue.submit(1, &submitInfo, VK_NULL_HANDLE);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command buffer to graphics queue! Error Code: " + vk::to_string(result));
    }
    queue.waitIdle();

    logicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

uint32_t Utilities::findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) 
{
    vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    physicalDevice.getMemoryProperties(&physicalDeviceMemoryProperties);

    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void Utilities::createBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer &buffer, vk::DeviceMemory &bufferMemory)
{
    vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo()
                                                .setSize(size)
                                                .setUsage(usage)
                                                .setSharingMode(vk::SharingMode::eExclusive);

    vk::Result result = logicalDevice.createBuffer(&bufferCreateInfo, nullptr, &buffer);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create buffer! Error Code: " + vk::to_string(result));
    }

    vk::MemoryRequirements memoryRequirements;
    logicalDevice.getBufferMemoryRequirements(buffer, &memoryRequirements);

    vk::MemoryAllocateInfo memoryAllocateInfo = vk::MemoryAllocateInfo()
                                                    .setAllocationSize(memoryRequirements.size)
                                                    .setMemoryTypeIndex(findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties));

    result = logicalDevice.allocateMemory(&memoryAllocateInfo, nullptr, &bufferMemory);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate buffer memory! Error Code: " + vk::to_string(result));
    }

    logicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

void Utilities::copyBuffer(vk::Device logicalDevice, vk::Queue queue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool commandPool)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(logicalDevice, commandPool);

    vk::BufferCopy copyRegion = vk::BufferCopy().setSize(size);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(logicalDevice, queue, commandBuffer, commandPool);
}