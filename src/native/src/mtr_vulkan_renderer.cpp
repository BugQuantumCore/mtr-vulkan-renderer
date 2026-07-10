#include "mtr_vulkan_renderer.h"
#include "vulkan_context.h"
#include "vulkan_pipeline.h"
#include "vulkan_buffer.h"
#include "vulkan_shader.h"
#include "vulkan_texture.h"
#include "mesh_manager.h"
#include "train_renderer.h"
#include "rail_renderer.h"

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <cstring>

// 句柄映射表
static std::unordered_map<uint64_t, std::unique_ptr<VulkanPipeline>> g_pipelines;
static std::unordered_map<uint64_t, std::unique_ptr<VulkanBuffer>> g_buffers;
static std::unordered_map<uint64_t, std::unique_ptr<VulkanTexture>> g_textures;

MTRVulkanRenderer::MTRVulkanRenderer() {}
MTRVulkanRenderer::~MTRVulkanRenderer() { cleanup(); }

bool MTRVulkanRenderer::initialize() {
    m_context = std::make_unique<VulkanContext>();
    if (!m_context->initialize()) {
        std::cerr << "[MTR-Vulkan] Failed to initialize Vulkan context" << std::endl;
        return false;
    }

    m_meshManager = std::make_unique<MeshManager>(m_context.get());

    m_trainRenderer = std::make_unique<TrainRenderer>(m_context.get(), m_meshManager.get());
    if (!m_trainRenderer->initialize()) {
        std::cerr << "[MTR-Vulkan] Failed to initialize train renderer" << std::endl;
        return false;
    }

    m_railRenderer = std::make_unique<RailRenderer>(m_context.get(), m_meshManager.get());
    if (!m_railRenderer->initialize()) {
        std::cerr << "[MTR-Vulkan] Failed to initialize rail renderer" << std::endl;
        return false;
    }

    std::cout << "[MTR-Vulkan] MTR Vulkan Renderer initialized successfully" << std::endl;
    return true;
}

void MTRVulkanRenderer::cleanup() {
    g_pipelines.clear();
    g_buffers.clear();
    g_textures.clear();

    if (m_railRenderer) m_railRenderer->cleanup();
    if (m_trainRenderer) m_trainRenderer->cleanup();
    if (m_meshManager) m_meshManager->cleanup();
    if (m_context) m_context->cleanup();

    m_railRenderer.reset();
    m_trainRenderer.reset();
    m_meshManager.reset();
    m_context.reset();
}

void MTRVulkanRenderer::onWindowResize(int width, int height) {
    // 更新视口 - 在下一帧开始时生效
    std::cout << "[MTR-Vulkan] Window resized to " << width << "x" << height << std::endl;
}

int MTRVulkanRenderer::beginFrame() {
    if (!m_context) return -1;
    m_currentFrame = m_context->beginFrame();
    m_drawCallsThisFrame = 0;
    m_trianglesThisFrame = 0;

    auto start = std::chrono::high_resolution_clock::now();
    // 帧开始时间记录（用于计算帧时间）
    static auto lastStart = start;
    m_frameTimeMs = std::chrono::duration<float, std::milli>(start - lastStart).count();
    lastStart = start;

    return m_currentFrame;
}

void MTRVulkanRenderer::endFrame() {
    if (!m_context) return;

    // 清理列车实例数据
    if (m_trainRenderer) m_trainRenderer->clearInstances();

    m_context->endFrame();
}

void MTRVulkanRenderer::waitForFrame() {
    if (!m_context) return;
    m_context->waitFrame();
}

uint64_t MTRVulkanRenderer::createPipeline(const char* vertShaderPath, const char* fragShaderPath,
                                             const char* vertexFormat, int topology,
                                             bool blendEnabled, bool depthTestEnabled) {
    if (!m_context) return 0;

    auto pipeline = std::make_unique<VulkanPipeline>(m_context.get());

    VulkanPipeline::CreateInfo info;
    info.vertexShaderPath = vertShaderPath;
    info.fragmentShaderPath = fragShaderPath;
    info.vertexFormat = vertexFormat;
    info.topology = static_cast<VkPrimitiveTopology>(topology);
    info.blendEnabled = blendEnabled;
    info.depthTestEnabled = depthTestEnabled;

    if (!pipeline->create(info)) {
        std::cerr << "[MTR-Vulkan] Failed to create pipeline" << std::endl;
        return 0;
    }

    uint64_t handle = generateHandle();
    g_pipelines[handle] = std::move(pipeline);
    return handle;
}

void MTRVulkanRenderer::destroyPipeline(uint64_t handle) {
    auto it = g_pipelines.find(handle);
    if (it != g_pipelines.end()) {
        it->second->destroy();
        g_pipelines.erase(it);
    }
}

uint64_t MTRVulkanRenderer::createVertexBuffer(int sizeBytes) {
    if (!m_context) return 0;
    auto buffer = std::make_unique<VulkanBuffer>(m_context.get());
    if (!buffer->create(VulkanBuffer::Type::Vertex, sizeBytes)) return 0;
    uint64_t handle = generateHandle();
    g_buffers[handle] = std::move(buffer);
    return handle;
}

uint64_t MTRVulkanRenderer::createIndexBuffer(int sizeBytes) {
    if (!m_context) return 0;
    auto buffer = std::make_unique<VulkanBuffer>(m_context.get());
    if (!buffer->create(VulkanBuffer::Type::Index, sizeBytes)) return 0;
    uint64_t handle = generateHandle();
    g_buffers[handle] = std::move(buffer);
    return handle;
}

uint64_t MTRVulkanRenderer::createUniformBuffer(int sizeBytes) {
    if (!m_context) return 0;
    auto buffer = std::make_unique<VulkanBuffer>(m_context.get());
    if (!buffer->create(VulkanBuffer::Type::Uniform, sizeBytes)) return 0;
    uint64_t handle = generateHandle();
    g_buffers[handle] = std::move(buffer);
    return handle;
}

void MTRVulkanRenderer::uploadVertexData(uint64_t handle, const float* data, int count, int offsetBytes) {
    auto it = g_buffers.find(handle);
    if (it != g_buffers.end()) {
        it->second->upload(data, count * sizeof(float), offsetBytes);
    }
}

void MTRVulkanRenderer::uploadIndexData(uint64_t handle, const int* data, int count, int offsetBytes) {
    auto it = g_buffers.find(handle);
    if (it != g_buffers.end()) {
        it->second->upload(data, count * sizeof(int), offsetBytes);
    }
}

void MTRVulkanRenderer::uploadUniformData(uint64_t handle, const uint8_t* data, int size, int offsetBytes) {
    auto it = g_buffers.find(handle);
    if (it != g_buffers.end()) {
        it->second->upload(data, size, offsetBytes);
    }
}

void MTRVulkanRenderer::destroyBuffer(uint64_t handle) {
    auto it = g_buffers.find(handle);
    if (it != g_buffers.end()) {
        it->second->destroy();
        g_buffers.erase(it);
    }
}

uint64_t MTRVulkanRenderer::createTexture(const uint8_t* pixelData, int width, int height, int mipLevels) {
    if (!m_context) return 0;
    auto texture = std::make_unique<VulkanTexture>(m_context.get());
    if (!texture->create(pixelData, width, height, mipLevels)) return 0;
    uint64_t handle = generateHandle();
    g_textures[handle] = std::move(texture);
    return handle;
}

uint64_t MTRVulkanRenderer::createTextureFromGL(int glTextureId, int width, int height) {
    if (!m_context) return 0;
    auto texture = std::make_unique<VulkanTexture>(m_context.get());
    if (!texture->createFromGL(glTextureId, width, height)) return 0;
    uint64_t handle = generateHandle();
    g_textures[handle] = std::move(texture);
    return handle;
}

void MTRVulkanRenderer::updateTexture(uint64_t handle, const uint8_t* data, int x, int y, int w, int h) {
    auto it = g_textures.find(handle);
    if (it != g_textures.end()) {
        it->second->update(data, x, y, w, h);
    }
}

void MTRVulkanRenderer::destroyTexture(uint64_t handle) {
    auto it = g_textures.find(handle);
    if (it != g_textures.end()) {
        it->second->destroy();
        g_textures.erase(it);
    }
}

void MTRVulkanRenderer::bindPipeline(uint64_t handle) {
    auto it = g_pipelines.find(handle);
    if (it != g_pipelines.end()) {
        VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE) {
            it->second->bind(cmd);
        }
    }
}

void MTRVulkanRenderer::bindVertexBuffer(uint64_t handle) {
    auto it = g_buffers.find(handle);
    if (it != g_buffers.end()) {
        VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE) {
            VkBuffer buffer = it->second->getBuffer();
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &buffer, &offset);
        }
    }
}

void MTRVulkanRenderer::bindIndexBuffer(uint64_t handle) {
    auto it = g_buffers.find(handle);
    if (it != g_buffers.end()) {
        VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE) {
            vkCmdBindIndexBuffer(cmd, it->second->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }
}

void MTRVulkanRenderer::bindTexture(uint64_t handle, int slot) {
    // 纹理通过描述符集绑定
    auto it = g_textures.find(handle);
    if (it != g_textures.end()) {
        // 更新描述符集中的纹理引用
        // 实际实现需要管理描述符集
    }
}

void MTRVulkanRenderer::setUniforms(const float* mvpMatrix, const float* modelMatrix,
                                      const float* lightDirection, float ambientLight) {
    // 将Uniform数据写入Uniform缓冲区
    struct UniformData {
        float mvp[16];
        float model[16];
        float lightDir[4]; // vec4 padding
        float ambient;
        float padding[3];
    };

    // 找到当前帧的Uniform缓冲区并更新
    // 简化实现：通过push constants传递
}

void MTRVulkanRenderer::drawIndexed(int indexCount, int instanceCount) {
    VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
    if (cmd != VK_NULL_HANDLE) {
        vkCmdDrawIndexed(cmd, indexCount, instanceCount, 0, 0, 0);
        m_drawCallsThisFrame++;
        m_trianglesThisFrame += (indexCount / 3) * instanceCount;
    }
}

void MTRVulkanRenderer::drawArrays(int vertexCount, int instanceCount) {
    VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
    if (cmd != VK_NULL_HANDLE) {
        vkCmdDraw(cmd, vertexCount, instanceCount, 0, 0);
        m_drawCallsThisFrame++;
        m_trianglesThisFrame += (vertexCount / 3) * instanceCount;
    }
}

void MTRVulkanRenderer::drawIndirect(const int* commands, int commandCount) {
    // GPU驱动渲染 - 需要间接绘制缓冲区
    // 暂不实现
}

void MTRVulkanRenderer::registerMesh(const char* meshId, const float* vertices, int vertexCount,
                                       const int* indices, int indexCount, uint64_t textureHandle) {
    if (m_meshManager) {
        m_meshManager->registerMesh(meshId, vertices, vertexCount, indices, indexCount, textureHandle);
    }
}

void MTRVulkanRenderer::renderMesh(const char* meshId, const float* mvpMatrix, const float* color) {
    VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
    if (cmd != VK_NULL_HANDLE && m_meshManager) {
        m_meshManager->renderMesh(cmd, meshId, 0, mvpMatrix, color);
        m_drawCallsThisFrame++;

        const auto* mesh = m_meshManager->getMesh(meshId);
        if (mesh) {
            m_trianglesThisFrame += mesh->triangleCount;
        }
    }
}

void MTRVulkanRenderer::unregisterMesh(const char* meshId) {
    if (m_meshManager) {
        m_meshManager->unregisterMesh(meshId);
    }
}

std::string MTRVulkanRenderer::getGPUInfo() const {
    if (!m_context) return "Not initialized";
    return m_context->getGPUName();
}

void MTRVulkanRenderer::getRenderStats(float* stats) const {
    stats[0] = static_cast<float>(m_drawCallsThisFrame);
    stats[1] = static_cast<float>(m_trianglesThisFrame);
    stats[2] = m_frameTimeMs;
    stats[3] = 0; // GPU内存使用量（需要VMA统计）
}

void MTRVulkanRenderer::debugMarker(const char* name, float r, float g, float b) {
    VkCommandBuffer cmd = m_context->getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    // 使用 VK_EXT_debug_utils 插入调试标记
    // 需要检查扩展是否可用
    // PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = ...;
    // VkDebugUtilsLabelEXT label = {};
    // label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    // label.pLabelName = name;
    // label.color[0] = r; label.color[1] = g; label.color[2] = b; label.color[3] = 1.0f;
    // vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
}