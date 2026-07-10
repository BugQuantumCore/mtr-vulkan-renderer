#pragma once

#include "vulkan_context.h"
#include "mesh_manager.h"
#include <vector>

/**
 * C++侧的列车渲染器。
 * 管理列车的实例化渲染，优化大量列车的绘制。
 */
class TrainRenderer {
public:
    struct TrainInstance {
        float modelMatrix[16];  // 4x4模型变换矩阵
        float color[4];         // 颜色叠加
        float extra[4];         // 额外数据（车灯光照等）
    };

    TrainRenderer(VulkanContext* context, MeshManager* meshManager);
    ~TrainRenderer();

    bool initialize();

    /**
     * 添加一个列车实例用于渲染
     */
    void addInstance(const std::string& trainType, const TrainInstance& instance);

    /**
     * 提交所有实例并执行实例化渲染
     */
    void render(VkCommandBuffer cmd, uint64_t pipelineHandle, const float* vpMatrix);

    /**
     * 清除当前帧的实例数据
     */
    void clearInstances();

    void cleanup();

private:
    VulkanContext* m_context;
    MeshManager* m_meshManager;

    // 实例缓冲区（每帧更新）
    std::unique_ptr<VulkanBuffer> m_instanceBuffer;
    static constexpr int MAX_INSTANCES = 1024;

    // 按类型分组的实例
    std::unordered_map<std::string, std::vector<TrainInstance>> m_instancesByType;
};