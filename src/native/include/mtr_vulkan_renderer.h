#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// 前向声明
class VulkanContext;
class VulkanPipeline;
class VulkanBuffer;
class MeshManager;
class TrainRenderer;
class RailRenderer;

/**
 * MTR Vulkan渲染器的主管理类。
 * 管理所有Vulkan资源和渲染子系统的生命周期。
 */
class MTRVulkanRenderer {
public:
    MTRVulkanRenderer();
    ~MTRVulkanRenderer();

    // 初始化/清理
    bool initialize();
    void cleanup();
    void onWindowResize(int width, int height);

    // 帧管理
    int beginFrame();
    void endFrame();
    void waitForFrame();

    // 管线管理
    uint64_t createPipeline(const char* vertShaderPath, const char* fragShaderPath,
                           const char* vertexFormat, int topology,
                           bool blendEnabled, bool depthTestEnabled);
    void destroyPipeline(uint64_t handle);

    // 缓冲区管理
    uint64_t createVertexBuffer(int sizeBytes);
    uint64_t createIndexBuffer(int sizeBytes);
    uint64_t createUniformBuffer(int sizeBytes);
    void uploadVertexData(uint64_t handle, const float* data, int count, int offsetBytes);
    void uploadIndexData(uint64_t handle, const int* data, int count, int offsetBytes);
    void uploadUniformData(uint64_t handle, const uint8_t* data, int size, int offsetBytes);
    void destroyBuffer(uint64_t handle);

    // 纹理管理
    uint64_t createTexture(const uint8_t* pixelData, int width, int height, int mipLevels);
    uint64_t createTextureFromGL(int glTextureId, int width, int height);
    void updateTexture(uint64_t handle, const uint8_t* data, int x, int y, int w, int h);
    void destroyTexture(uint64_t handle);

    // 渲染
    void bindPipeline(uint64_t handle);
    void bindVertexBuffer(uint64_t handle);
    void bindIndexBuffer(uint64_t handle);
    void bindTexture(uint64_t handle, int slot);
    void setUniforms(const float* mvpMatrix, const float* modelMatrix,
                     const float* lightDirection, float ambientLight);
    void drawIndexed(int indexCount, int instanceCount);
    void drawArrays(int vertexCount, int instanceCount);
    void drawIndirect(const int* commands, int commandCount);

    // 网格管理
    void registerMesh(const char* meshId, const float* vertices, int vertexCount,
                      const int* indices, int indexCount, uint64_t textureHandle);
    void renderMesh(const char* meshId, const float* mvpMatrix, const float* color);
    void unregisterMesh(const char* meshId);

    // 调试
    std::string getGPUInfo() const;
    void getRenderStats(float* stats) const;
    void debugMarker(const char* name, float r, float g, float b);

private:
    std::unique_ptr<VulkanContext> m_context;
    std::unique_ptr<MeshManager> m_meshManager;
    std::unique_ptr<TrainRenderer> m_trainRenderer;
    std::unique_ptr<RailRenderer> m_railRenderer;

    uint64_t m_nextHandle = 1;
    uint64_t generateHandle() { return m_nextHandle++; }

    // 渲染统计
    int m_drawCallsThisFrame = 0;
    int m_trianglesThisFrame = 0;
    float m_frameTimeMs = 0;
    int m_currentFrame = -1;
};