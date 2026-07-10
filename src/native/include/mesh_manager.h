#pragma once

#include "vulkan_context.h"
#include "vulkan_buffer.h"
#include <string>
#include <unordered_map>
#include <memory>

/**
 * 管理所有静态网格。
 * 网格数据上传到GPU后保持不变，渲染时只需发送变换矩阵。
 */
class MeshManager {
public:
    struct Mesh {
        std::unique_ptr<VulkanBuffer> vertexBuffer;
        std::unique_ptr<VulkanBuffer> indexBuffer;
        uint64_t textureHandle = 0;
        int vertexCount = 0;
        int indexCount = 0;
        int triangleCount = 0;
    };

    MeshManager(VulkanContext* context);
    ~MeshManager();

    /**
     * 注册一个网格
     * @param id 唯一标识符
     * @param vertices 交错顶点数据 (pos3 + normal3 + uv2 + color4 = 12 floats)
     * @param vertexCount 顶点数量
     * @param indices 索引数据
     * @param indexCount 索引数量
     * @param textureHandle 纹理句柄
     */
    bool registerMesh(const std::string& id,
                      const float* vertices, int vertexCount,
                      const int* indices, int indexCount,
                      uint64_t textureHandle);

    /**
     * 渲染一个已注册的网格
     */
    void renderMesh(VkCommandBuffer cmd, const std::string& id,
                    uint64_t pipelineHandle,
                    const float* mvpMatrix, const float* color);

    void unregisterMesh(const std::string& id);
    bool hasMesh(const std::string& id) const;
    const Mesh* getMesh(const std::string& id) const;

    void cleanup();

private:
    VulkanContext* m_context;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
};