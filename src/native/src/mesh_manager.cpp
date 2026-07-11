#include "mesh_manager.h"
#include <iostream>
#include <cstring>
#include <bits/stdc++.h>

MeshManager::MeshManager(VulkanContext* context)
    : m_context(context) {}

MeshManager::~MeshManager() {
    cleanup();
}

bool MeshManager::registerMesh(const std::string& id,
                                const float* vertices, int vertexCount,
                                const int* indices, int indexCount,
                                uint64_t textureHandle) {
    // 检查是否已存在
    if (m_meshes.find(id) != m_meshes.end()) {
        std::cerr << "[MTR-Vulkan] Mesh already registered: " << id << std::endl;
        return false;
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->vertexCount = vertexCount;
    mesh->indexCount = indexCount;
    mesh->triangleCount = indexCount / 3;
    mesh->textureHandle = textureHandle;

    // 创建顶点缓冲区
    // 每个顶点: 3(pos) + 3(normal) + 2(uv) + 4(color) = 12 floats = 48 bytes
    VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(vertexCount) * 48;
    mesh->vertexBuffer = std::make_unique<VulkanBuffer>(m_context);
    if (!mesh->vertexBuffer->create(VulkanBuffer::Type::Vertex, vertexBufferSize)) {
        std::cerr << "[MTR-Vulkan] Failed to create vertex buffer for mesh: " << id << std::endl;
        return false;
    }
    mesh->vertexBuffer->upload(vertices, vertexBufferSize);

    // 创建索引缓冲区
    VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(indexCount) * sizeof(int);
    mesh->indexBuffer = std::make_unique<VulkanBuffer>(m_context);
    if (!mesh->indexBuffer->create(VulkanBuffer::Type::Index, indexBufferSize)) {
        std::cerr << "[MTR-Vulkan] Failed to create index buffer for mesh: " << id << std::endl;
        return false;
    }
    mesh->indexBuffer->upload(indices, indexBufferSize);

    m_meshes[id] = std::move(mesh);

    std::cout << "[MTR-Vulkan] Registered mesh: " << id
              << " (" << vertexCount << " vertices, " << indexCount << " indices)" << std::endl;
    return true;
}

void MeshManager::renderMesh(VkCommandBuffer cmd, const std::string& id,
                              uint64_t pipelineHandle,
                              const float* mvpMatrix, const float* color) {
    auto it = m_meshes.find(id);
    if (it == m_meshes.end()) {
        return;
    }

    const auto& mesh = it->second;

    // 推送变换矩阵和颜色作为Push Constants
    struct PushData {
        float mvp[16];
        float color[4];
    } pushData;

    memcpy(pushData.mvp, mvpMatrix, sizeof(float) * 16);
    memcpy(pushData.color, color, sizeof(float) * 4);

    vkCmdPushConstants(cmd,
            VK_NULL_HANDLE, // pipelineLayout - 由调用者绑定管线后获取
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PushData), &pushData);

    // 绑定顶点缓冲区
    VkBuffer vertexBuffer = mesh->vertexBuffer->getBuffer();
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);

    // 绑定索引缓冲区
    vkCmdBindIndexBuffer(cmd, mesh->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    // 绘制
    vkCmdDrawIndexed(cmd, mesh->indexCount, 1, 0, 0, 0);
}

void MeshManager::unregisterMesh(const std::string& id) {
    auto it = m_meshes.find(id);
    if (it != m_meshes.end()) {
        m_meshes.erase(it);
    }
}

bool MeshManager::hasMesh(const std::string& id) const {
    return m_meshes.find(id) != m_meshes.end();
}

const MeshManager::Mesh* MeshManager::getMesh(const std::string& id) const {
    auto it = m_meshes.find(id);
    return (it != m_meshes.end()) ? it->second.get() : nullptr;
}

void MeshManager::cleanup() {
    m_meshes.clear(); // unique_ptr自动清理缓冲区
    std::cout << "[MTR-Vulkan] Mesh manager cleaned up" << std::endl;
}