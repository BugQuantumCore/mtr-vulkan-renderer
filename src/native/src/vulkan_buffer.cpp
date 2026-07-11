#include "vulkan_buffer.h"
#include <iostream>
#include <cstring>
#include <bits/stdc++.h>

VulkanBuffer::VulkanBuffer(VulkanContext* context)
    : m_context(context) {}

VulkanBuffer::~VulkanBuffer() {
    destroy();
}

bool VulkanBuffer::create(Type type, VkDeviceSize size,
                           VkBufferUsageFlags additionalUsage,
                           VmaMemoryUsage memoryUsage) {
    m_type = type;
    m_size = size;

    VkBufferUsageFlags usage = additionalUsage;
    switch (type) {
        case Type::Vertex:
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
        case Type::Index:
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case Type::Uniform:
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            memoryUsage = VMA_MEMORY_USAGE_AUTO; // Uniform缓冲区需要CPU可写
            break;
        case Type::Storage:
            usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case Type::Indirect:
            usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
    }

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;

    // 对于需要频繁CPU更新的缓冲区（如Uniform），要求映射
    if (type == Type::Uniform) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else if (type == Type::Storage || type == Type::Indirect) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VkResult result = vmaCreateBuffer(m_context->getAllocator(),
            &bufferInfo, &allocInfo, &m_buffer, &m_allocation, &m_allocInfo);

    if (result != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create buffer (type="
                  << static_cast<int>(type) << ", size=" << size << ")" << std::endl;
        return false;
    }

    // 如果VMA已经映射了内存，保存指针
    if (m_allocInfo.pMappedData) {
        m_mappedData = m_allocInfo.pMappedData;
    }

    return true;
}

void* VulkanBuffer::map() {
    if (m_mappedData) return m_mappedData;

    VkResult result = vmaMapMemory(m_context->getAllocator(), m_allocation, &m_mappedData);
    if (result != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to map buffer memory" << std::endl;
        return nullptr;
    }
    return m_mappedData;
}

void VulkanBuffer::unmap() {
    if (m_mappedData && !m_allocInfo.pMappedData) {
        // 只有在VMA没有永久映射时才手动unmap
        vmaUnmapMemory(m_context->getAllocator(), m_allocation);
        m_mappedData = nullptr;
    }
}

void VulkanBuffer::upload(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (size > m_size - offset) {
        std::cerr << "[MTR-Vulkan] Buffer upload exceeds size: "
                  << size << " > " << (m_size - offset) << std::endl;
        return;
    }

    // 对于小的或频繁更新的数据，直接memcpy到映射的内存
    void* mapped = map();
    if (mapped) {
        memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);

        // 刷新非host-coherent内存
        vmaFlushAllocation(m_context->getAllocator(), m_allocation, offset, size);
    } else {
        std::cerr << "[MTR-Vulkan] Failed to map buffer for upload" << std::endl;
    }
}

void VulkanBuffer::destroy() {
    if (m_buffer != VK_NULL_HANDLE) {
        unmap();
        vmaDestroyBuffer(m_context->getAllocator(), m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
        m_mappedData = nullptr;
        m_size = 0;
    }
}