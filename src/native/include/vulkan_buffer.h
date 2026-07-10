#pragma once

#include "vulkan_context.h"

/**
 * Vulkan缓冲区封装（使用VMA进行内存管理）
 */
class VulkanBuffer {
public:
    enum class Type {
        Vertex,
        Index,
        Uniform,
        Storage,
        Indirect
    };

    VulkanBuffer(VulkanContext* context);
    ~VulkanBuffer();

    bool create(Type type, VkDeviceSize size,
                VkBufferUsageFlags additionalUsage = 0,
                VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    void* map();
    void unmap();
    void upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    VkBuffer getBuffer() const { return m_buffer; }
    VkDeviceSize getSize() const { return m_size; }
    Type getType() const { return m_type; }

    void destroy();

private:
    VulkanContext* m_context;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_allocInfo = {};
    VkDeviceSize m_size = 0;
    Type m_type;
    void* m_mappedData = nullptr;
};