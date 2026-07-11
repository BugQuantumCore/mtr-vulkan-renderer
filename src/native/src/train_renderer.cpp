#include "train_renderer.h"
#include <iostream>
#include <cstring>

TrainRenderer::TrainRenderer(VulkanContext* context, MeshManager* meshManager)
    : m_context(context), m_meshManager(meshManager) {}

TrainRenderer::~TrainRenderer() {
    cleanup();
}

bool TrainRenderer::initialize() {
    // 创建实例缓冲区
    // 每个实例: 4x4模型矩阵(64字节) + 颜色(16字节) + 额外数据(16字节) = 96字节
    VkDeviceSize bufferSize = sizeof(TrainInstance) * MAX_INSTANCES;

    m_instanceBuffer = std::make_unique<VulkanBuffer>(m_context);
    if (!m_instanceBuffer->create(VulkanBuffer::Type::Storage, bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST)) {
        std::cerr << "[MTR-Vulkan] Failed to create train instance buffer" << std::endl;
        return false;
    }

    std::cout << "[MTR-Vulkan] Train renderer initialized (max " << MAX_INSTANCES << " instances)" << std::endl;
    return true;
}

void TrainRenderer::addInstance(const std::string& trainType, const TrainInstance& instance) {
    m_instancesByType[trainType].push_back(instance);
}

void TrainRenderer::render(VkCommandBuffer cmd, uint64_t pipelineHandle, const float* vpMatrix) {
    if (m_instancesByType.empty()) return;

    // 按列车类型分组渲染（每种类型使用相同的网格和纹理）
    for (auto& [trainType, instances] : m_instancesByType) {
        if (instances.empty()) continue;

        // 上传实例数据到GPU
        VkDeviceSize dataSize = sizeof(TrainInstance) * instances.size();
        if (dataSize > sizeof(TrainInstance) * MAX_INSTANCES) {
            dataSize = sizeof(TrainInstance) * MAX_INSTANCES;
            instances.resize(MAX_INSTANCES);
        }
        m_instanceBuffer->upload(instances.data(), dataSize);

        // 获取该类型列车的网格
        std::string meshId = "train_" + trainType;
        if (!m_meshManager->hasMesh(meshId)) continue;

        const auto* mesh = m_meshManager->getMesh(meshId);
        if (!mesh) continue;

        // 对于实例化渲染，我们需要使用实例缓冲区中的变换数据
        // 而不是push constants（push constants只能传递单个实例的数据）

        // 绑定顶点缓冲区
        VkBuffer vertexBuffer = mesh->vertexBuffer->getBuffer();
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);

        // 绑定索引缓冲区
        vkCmdBindIndexBuffer(cmd, mesh->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // 推送VP矩阵（对所有实例通用）
        struct VPPushData {
            float vp[16];
        } vpPush;
        memcpy(vpPush.vp, vpMatrix, sizeof(float) * 16);

        vkCmdPushConstants(cmd, VK_NULL_HANDLE,
                VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VPPushData), &vpPush);

        // 实例化绘制
        int instanceCount = static_cast<int>(instances.size());
        vkCmdDrawIndexed(cmd, mesh->indexCount, instanceCount, 0, 0, 0);
    }
}

void TrainRenderer::clearInstances() {
    m_instancesByType.clear();
}

void TrainRenderer::cleanup() {
    m_instanceBuffer.reset();
    m_instancesByType.clear();
}