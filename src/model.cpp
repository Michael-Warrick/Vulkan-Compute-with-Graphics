#include "model.hpp"

void Model::Load(const char *modelPath, vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::Queue queue, vk::CommandPool commandPool)
{
    loadModel(modelPath);
    createVertexBuffer(physicalDevice, logicalDevice, queue, commandPool);
    createIndexBuffer(physicalDevice, logicalDevice, queue, commandPool);
}

void Model::Draw(vk::CommandBuffer commandBuffer) 
{
    vk::Buffer vertexBuffers[] = {vertexBuffer};
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Model::Destroy(vk::Device logicalDevice) 
{
    logicalDevice.destroyBuffer(indexBuffer);
    logicalDevice.freeMemory(indexBufferMemory);

    logicalDevice.destroyBuffer(vertexBuffer);
    logicalDevice.freeMemory(vertexBufferMemory);
}

void Model::loadModel(const char *path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning, error;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, path))
    {
        throw std::runtime_error(warning + error);
    }

    std::unordered_map<Vertex, uint32_t, VertexHasher> uniqueVertices{};

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};

            vertex.textureCoordinates = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void Model::createVertexBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::Queue queue, vk::CommandPool commandPool)
{
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    Utilities::createBuffer(physicalDevice, logicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vk::Result result = logicalDevice.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map vertex buffer memory! Error Code: " + vk::to_string(result));
    }

    memcpy(data, vertices.data(), (size_t)bufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    Utilities::createBuffer(physicalDevice, logicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
    Utilities::copyBuffer(logicalDevice, queue, stagingBuffer, vertexBuffer, bufferSize, commandPool);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}

void Model::createIndexBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::Queue queue, vk::CommandPool commandPool)
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    Utilities::createBuffer(physicalDevice, logicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory);

    void *data;
    vk::Result result = logicalDevice.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map vertex buffer memory! Error Code: " + vk::to_string(result));
    }

    memcpy(data, indices.data(), (size_t)bufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    Utilities::createBuffer(physicalDevice, logicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
    Utilities::copyBuffer(logicalDevice, queue, stagingBuffer, indexBuffer, bufferSize, commandPool);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}