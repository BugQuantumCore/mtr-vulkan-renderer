#pragma once

#include "vulkan_context.h"
#include "mesh_manager.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

/**
 * C++侧的轨道渲染器
 * 管理大量轨道区段的渲染，使用空间分区和LOD优化
 */
class RailRenderer {
public:
    struct RailSegment {
        float startX, startY, startZ;
        float endX, endY, endZ;
        float gauge;          // 轨距
        int segmentType;      // 0=直线, 1=曲线, 2=道岔
        float curveControl[6]; // 贝塞尔曲线控制点 (x1,y1,z1,x2,y2,z2)
    };

    RailRenderer(VulkanContext* context, MeshManager* meshManager);
    ~RailRenderer();

    bool initialize();

    /**
     * 注册一段轨道
     */
    void registerSegment(int chunkX, int chunkZ, const RailSegment& segment);

    /**
     * 渲染指定范围内的轨道
     */
    void render(VkCommandBuffer cmd, uint64_t pipelineHandle,
                const float* vpMatrix, double camX, double camY, double camZ,
                int renderDistance);

    /**
     * 生成一个区块的轨道网格
     */
    void buildChunkMesh(int chunkX, int chunkZ);

    /**
     * 清除已卸载区块的网格
     */
    void unloadChunk(int chunkX, int chunkZ);

    void cleanup();

private:
    /**
     * 生成直线轨道的网格
     */
    void generateStraightRail(std::vector<float>& vertices, std::vector<int>& indices,
                               const RailSegment& seg);

    /**
     * 生成曲线轨道的网格（贝塞尔曲线）
     */
    void generateCurvedRail(std::vector<float>& vertices, std::vector<int>& indices,
                             const RailSegment& seg, int subdivisions);

    /**
     * 生成枕木
     */
    void generateSleepers(std::vector<float>& vertices, std::vector<int>& indices,
                           float startX, float startZ, float endX, float endZ,
                           float spacing, float width);

    static uint64_t chunkKey(int x, int z) {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint32_t>(z);
    }

    VulkanContext* m_context;
    MeshManager* m_meshManager;

    // 按区块组织的轨道数据
    std::unordered_map<uint64_t, std::vector<RailSegment>> m_chunkSegments;

    // 已构建的区块网格ID
    std::unordered_map<uint64_t, std::string> m_chunkMeshIds;

    // 预生成的标准轨道部件（用于实例化渲染）
    std::unique_ptr<VulkanBuffer> m_sleeperVertexBuffer;
    std::unique_ptr<VulkanBuffer> m_sleeperIndexBuffer;
    int m_sleeperIndexCount = 0;
};