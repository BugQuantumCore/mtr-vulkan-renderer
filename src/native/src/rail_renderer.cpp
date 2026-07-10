#include "rail_renderer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

RailRenderer::RailRenderer(VulkanContext* context, MeshManager* meshManager)
    : m_context(context), m_meshManager(meshManager) {}

RailRenderer::~RailRenderer() {
    cleanup();
}

bool RailRenderer::initialize() {
    // 预生成标准枕木的网格（用于实例化渲染）
    std::vector<float> sleeperVerts;
    std::vector<int> sleeperIndices;

    float hw = 1.25f;  // 枕木半宽
    float hh = 0.075f; // 枕木半高
    float hd = 0.05f;  // 枕木半深

    // 枕木: 2.5m x 0.15m x 0.1m
    // 顶面
    int base = 0;
    auto addVert = [&](float x, float y, float z, float nx, float ny, float nz) {
        sleeperVerts.push_back(x); sleeperVerts.push_back(y); sleeperVerts.push_back(z);
        sleeperVerts.push_back(nx); sleeperVerts.push_back(ny); sleeperVerts.push_back(nz);
        sleeperVerts.push_back(0); sleeperVerts.push_back(0); // UV
        sleeperVerts.push_back(0.4f); sleeperVerts.push_back(0.3f);
        sleeperVerts.push_back(0.2f); sleeperVerts.push_back(1.0f); // 木色
    };

    // 6面长方体
    // 顶面
    addVert(-hw, hh, -hd, 0, 1, 0); addVert(hw, hh, -hd, 0, 1, 0);
    addVert(hw, hh, hd, 0, 1, 0);   addVert(-hw, hh, hd, 0, 1, 0);
    sleeperIndices.insert(sleeperIndices.end(), {0,1,2, 0,2,3});

    // 底面
    addVert(-hw, -hh, -hd, 0, -1, 0); addVert(-hw, -hh, hd, 0, -1, 0);
    addVert(hw, -hh, hd, 0, -1, 0);   addVert(hw, -hh, -hd, 0, -1, 0);
    sleeperIndices.insert(sleeperIndices.end(), {4,5,6, 4,6,7});

    // 前面
    addVert(-hw, -hh, -hd, 0, 0, -1); addVert(hw, -hh, -hd, 0, 0, -1);
    addVert(hw, hh, -hd, 0, 0, -1);   addVert(-hw, hh, -hd, 0, 0, -1);
    sleeperIndices.insert(sleeperIndices.end(), {8,9,10, 8,10,11});

    // 后面
    addVert(-hw, -hh, hd, 0, 0, 1); addVert(-hw, hh, hd, 0, 0, 1);
    addVert(hw, hh, hd, 0, 0, 1);   addVert(hw, -hh, hd, 0, 0, 1);
    sleeperIndices.insert(sleeperIndices.end(), {12,13,14, 12,14,15});

    // 左面
    addVert(-hw, -hh, -hd, -1, 0, 0); addVert(-hw, hh, -hd, -1, 0, 0);
    addVert(-hw, hh, hd, -1, 0, 0);   addVert(-hw, -hh, hd, -1, 0, 0);
    sleeperIndices.insert(sleeperIndices.end(), {16,17,18, 16,18,19});

    // 右面
    addVert(hw, -hh, -hd, 1, 0, 0); addVert(hw, -hh, hd, 1, 0, 0);
    addVert(hw, hh, hd, 1, 0, 0);   addVert(hw, hh, -hd, 1, 0, 0);
    sleeperIndices.insert(sleeperIndices.end(), {20,21,22, 20,22,23});

    m_sleeperIndexCount = static_cast<int>(sleeperIndices.size());

    // 上传枕木网格
    VkDeviceSize vSize = sleeperVerts.size() * sizeof(float);
    VkDeviceSize iSize = sleeperIndices.size() * sizeof(int);

    m_sleeperVertexBuffer = std::make_unique<VulkanBuffer>(m_context);
    m_sleeperVertexBuffer->create(VulkanBuffer::Type::Vertex, vSize);
    m_sleeperVertexBuffer->upload(sleeperVerts.data(), vSize);

    m_sleeperIndexBuffer = std::make_unique<VulkanBuffer>(m_context);
    m_sleeperIndexBuffer->create(VulkanBuffer::Type::Index, iSize);
    m_sleeperIndexBuffer->upload(sleeperIndices.data(), iSize);

    std::cout << "[MTR-Vulkan] Rail renderer initialized" << std::endl;
    return true;
}

void RailRenderer::registerSegment(int chunkX, int chunkZ, const RailSegment& segment) {
    uint64_t key = chunkKey(chunkX, chunkZ);
    m_chunkSegments[key].push_back(segment);
}

void RailRenderer::render(VkCommandBuffer cmd, uint64_t pipelineHandle,
                           const float* vpMatrix, double camX, double camY, double camZ,
                           int renderDistance) {
    float identityColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (auto& [key, meshId] : m_chunkMeshIds) {
        int cx = static_cast<int>(key >> 32);
        int cz = static_cast<int>(key & 0xFFFFFFFF);

        // 简单的距离裁剪
        double dx = (cx * 16 + 8) - camX;
        double dz = (cz * 16 + 8) - camZ;
        double dist = std::sqrt(dx * dx + dz * dz);
        if (dist > renderDistance) continue;

        // 构建该区块的模型矩阵（平移到区块世界坐标）
        float modelMatrix[16];
        memset(modelMatrix, 0, sizeof(modelMatrix));
        modelMatrix[0] = 1.0f;
        modelMatrix[5] = 1.0f;
        modelMatrix[10] = 1.0f;
        modelMatrix[15] = 1.0f;
        modelMatrix[12] = static_cast<float>(cx * 16.0 - camX);
        modelMatrix[13] = static_cast<float>(-camY);
        modelMatrix[14] = static_cast<float>(cz * 16.0 - camZ);

        // 计算MVP
        float mvpMatrix[16];
        // 简化：直接传入VP * Model
        // 实际应该做矩阵乘法
        // 这里我们使用push constant传递
        m_meshManager->renderMesh(cmd, meshId, pipelineHandle, vpMatrix, identityColor);
    }
}

void RailRenderer::buildChunkMesh(int chunkX, int chunkZ) {
    uint64_t key = chunkKey(chunkX, chunkZ);

    auto segIt = m_chunkSegments.find(key);
    if (segIt == m_chunkSegments.end() || segIt->second.empty()) return;

    // 如果已经构建过，跳过
    if (m_chunkMeshIds.find(key) != m_chunkMeshIds.end()) return;

    std::vector<float> allVertices;
    std::vector<int> allIndices;

    for (const auto& seg : segIt->second) {
        int baseIndex = static_cast<int>(allVertices.size() / 12);

        if (seg.segmentType == 0) {
            generateStraightRail(allVertices, allIndices, seg);
        } else if (seg.segmentType == 1) {
            generateCurvedRail(allVertices, allIndices, seg, 16);
        }
    }

    if (allVertices.empty()) return;

    // 注册到网格管理器
    std::string meshId = "rail_chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ);
    int vertexCount = static_cast<int>(allVertices.size() / 12);
    int indexCount = static_cast<int>(allIndices.size());

    if (m_meshManager->registerMesh(meshId, allVertices.data(), vertexCount,
            allIndices.data(), indexCount, 0)) {
        m_chunkMeshIds[key] = meshId;
    }
}

void RailRenderer::generateStraightRail(std::vector<float>& vertices, std::vector<int>& indices,
                                          const RailSegment& seg) {
    float gauge = seg.gauge > 0 ? seg.gauge : 1.435f;
    float halfGauge = gauge / 2.0f;
    float railHeight = 0.1f;
    float railWidth = 0.06f;

    float dx = seg.endX - seg.startX;
    float dz = seg.endZ - seg.startZ;
    float length = std::sqrt(dx * dx + dz * dz);
    if (length < 0.001f) return;

    float nx = -dz / length; // 垂直于轨道方向的法线
    float nz = dx / length;

    auto addVert = [&](float x, float y, float z, float nnx, float nny, float nnz,
                       float r, float g, float b) {
        vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
        vertices.push_back(nnx); vertices.push_back(nny); vertices.push_back(nnz);
        vertices.push_back(0); vertices.push_back(0);
        vertices.push_back(r); vertices.push_back(g); vertices.push_back(b); vertices.push_back(1.0f);
    };

    // 生成两条铁轨
    for (int rail = 0; rail < 2; rail++) {
        float sign = (rail == 0) ? 1.0f : -1.0f;
        float ox = nx * halfGauge * sign;
        float oz = nz * halfGauge * sign;

        int base = static_cast<int>(vertices.size() / 12);

        // 铁轨顶面
        addVert(seg.startX + ox - nx * railWidth, railHeight, seg.startZ + oz - nz * railWidth,
                0, 1, 0, 0.6f, 0.6f, 0.6f);
        addVert(seg.startX + ox + nx * railWidth, railHeight, seg.startZ + oz + nz * railWidth,
                0, 1, 0, 0.6f, 0.6f, 0.6f);
        addVert(seg.endX + ox + nx * railWidth, railHeight, seg.endZ + oz + nz * railWidth,
                0, 1, 0, 0.6f, 0.6f, 0.6f);
        addVert(seg.endX + ox - nx * railWidth, railHeight, seg.endZ + oz - nz * railWidth,
                0, 1, 0, 0.6f, 0.6f, 0.6f);

        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
    }

    // 生成枕木
    float sleeperSpacing = 0.6f;
    int sleeperCount = static_cast<int>(length / sleeperSpacing);
    generateSleepers(vertices, indices, seg.startX, seg.startZ, seg.endX, seg.endZ,
                     sleeperSpacing, gauge * 1.5f);
}

void RailRenderer::generateCurvedRail(std::vector<float>& vertices, std::vector<int>& indices,
                                        const RailSegment& seg, int subdivisions) {
    float gauge = seg.gauge > 0 ? seg.gauge : 1.435f;
    float halfGauge = gauge / 2.0f;
    float railHeight = 0.1f;
    float railWidth = 0.06f;

    // 三次贝塞尔曲线插值
    // P0 = start, P1 = control1, P2 = control2, P3 = end
    auto bezier = [](float t, float p0, float p1, float p2, float p3) -> float {
        float u = 1.0f - t;
        return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3;
    };

    float cx1 = seg.curveControl[0], cy1 = seg.curveControl[1], cz1 = seg.curveControl[2];
    float cx2 = seg.curveControl[3], cy2 = seg.curveControl[4], cz2 = seg.curveControl[5];

    // 采样曲线点
    std::vector<float> curveX(subdivisions + 1);
    std::vector<float> curveY(subdivisions + 1);
    std::vector<float> curveZ(subdivisions + 1);

    for (int i = 0; i <= subdivisions; i++) {
        float t = static_cast<float>(i) / subdivisions;
        curveX[i] = bezier(t, seg.startX, cx1, cx2, seg.endX);
        curveY[i] = bezier(t, seg.startY, cy1, cy2, seg.endY);
        curveZ[i] = bezier(t, seg.startZ, cz1, cz2, seg.endZ);
    }

    // 沿曲线生成铁轨
    for (int rail = 0; rail < 2; rail++) {
        float sign = (rail == 0) ? 1.0f : -1.0f;

        for (int i = 0; i < subdivisions; i++) {
            float dx = curveX[i + 1] - curveX[i];
            float dz = curveZ[i + 1] - curveZ[i];
            float segLen = std::sqrt(dx * dx + dz * dz);
            if (segLen < 0.001f) continue;

            float nx = -dz / segLen * halfGauge * sign;
            float nz = dx / segLen * halfGauge * sign;

            int base = static_cast<int>(vertices.size() / 12);

            // 两个顶点（简化为线段）
            vertices.push_back(curveX[i] + nx - dz/segLen * railWidth);
            vertices.push_back(curveY[i] + railHeight);
            vertices.push_back(curveZ[i] + nz + dx/segLen * railWidth);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(0); vertices.push_back(0);
            vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(1.0f);

            vertices.push_back(curveX[i] + nx + dz/segLen * railWidth);
            vertices.push_back(curveY[i] + railHeight);
            vertices.push_back(curveZ[i] + nz - dx/segLen * railWidth);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(0); vertices.push_back(0);
            vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(1.0f);

            vertices.push_back(curveX[i+1] + nx + dz/segLen * railWidth);
            vertices.push_back(curveY[i+1] + railHeight);
            vertices.push_back(curveZ[i+1] + nz - dx/segLen * railWidth);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(0); vertices.push_back(0);
            vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(1.0f);

            vertices.push_back(curveX[i+1] + nx - dz/segLen * railWidth);
            vertices.push_back(curveY[i+1] + railHeight);
            vertices.push_back(curveZ[i+1] + nz + dx/segLen * railWidth);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(0); vertices.push_back(0);
            vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(0.6f); vertices.push_back(1.0f);

            indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
        }
    }
}

void RailRenderer::generateSleepers(std::vector<float>& vertices, std::vector<int>& indices,
                                      float startX, float startZ, float endX, float endZ,
                                      float spacing, float width) {
    float dx = endX - startX;
    float dz = endZ - startZ;
    float length = std::sqrt(dx * dx + dz * dz);
    if (length < 0.001f) return;

    float dirX = dx / length;
    float dirZ = dz / length;
    float perpX = -dirZ;
    float perpZ = dirX;
    float halfWidth = width / 2.0f;
    float sleeperH = 0.1f;
    float sleeperD = 0.12f;

    int count = static_cast<int>(length / spacing);
    for (int i = 0; i < count; i++) {
        float t = (static_cast<float>(i) + 0.5f) / count;
        float cx = startX + dx * t;
        float cz = startZ + dz * t;

        int base = static_cast<int>(vertices.size() / 12);

        // 简化枕木为一个四边形（顶面）
        auto addSleeperVert = [&](float x, float y, float z) {
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(0); vertices.push_back(0);
            vertices.push_back(0.35f); vertices.push_back(0.25f);
            vertices.push_back(0.15f); vertices.push_back(1.0f);
        };

        addSleeperVert(cx - perpX * halfWidth, sleeperH, cz - perpZ * halfWidth);
        addSleeperVert(cx + perpX * halfWidth, sleeperH, cz + perpZ * halfWidth);
        addSleeperVert(cx + perpX * halfWidth + dirX * sleeperD, sleeperH, cz + perpZ * halfWidth + dirZ * sleeperD);
        addSleeperVert(cx - perpX * halfWidth + dirX * sleeperD, sleeperH, cz - perpZ * halfWidth + dirZ * sleeperD);

        indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
    }
}

void RailRenderer::unloadChunk(int chunkX, int chunkZ) {
    uint64_t key = chunkKey(chunkX, chunkZ);
    auto it = m_chunkMeshIds.find(key);
    if (it != m_chunkMeshIds.end()) {
        m_meshManager->unregisterMesh(it->second);
        m_chunkMeshIds.erase(it);
    }
}

void RailRenderer::cleanup() {
    for (auto& [key, meshId] : m_chunkMeshIds) {
        m_meshManager->unregisterMesh(meshId);
    }
    m_chunkMeshIds.clear();
    m_chunkSegments.clear();
    m_sleeperVertexBuffer.reset();
    m_sleeperIndexBuffer.reset();
}