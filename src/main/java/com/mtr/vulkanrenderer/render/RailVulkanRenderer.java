package com.mtr.vulkanrenderer.render;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.MeshData;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import com.mtr.vulkanrenderer.config.ModConfig;
import com.mtr.vulkanrenderer.vulkan.VulkanContext;
import com.mtr.vulkanrenderer.vulkan.VulkanTextureManager;
import net.minecraft.entity.player.PlayerEntity;
import org.joml.Matrix4f;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/**
 * 使用Vulkan渲染MTR的轨道/铁路。
 *
 * MTR的轨道渲染特点:
 * - 轨道由多个区段(Segment)组成
 * - 每个区段使用贝塞尔曲线定义路径
 * - 轨道模型沿路径生成网格（包括铁轨、枕木、道砟等）
 * - 支持不同轨道类型：普通轨、高架轨、地下轨等
 * - 渲染量巨大（一个大型铁路网络可能有数千个轨道区段）
 *
 * Vulkan优化策略:
 * - 静态轨道网格预烘焙到大型顶点缓冲区
 * - 使用实例化渲染重复的枕木和铁轨零件
 * - 基于距离的LOD系统
 * - 视锥体裁剪 + 遮挡剔除
 */
public class RailVulkanRenderer {

    private final VulkanContext vulkanContext;
    private final VulkanBridge bridge;
    private final VulkanTextureManager textureManager;
    private final ModConfig config;

    // 按区块缓存的轨道网格
    private final Map<Long, String> chunkRailMeshCache = new ConcurrentHashMap<>();

    // 可见的轨道区块
    private final Set<Long> visibleChunks = new HashSet<>();

    // 缓冲区
    private long railVertexBuffer;
    private long railIndexBuffer;
    private long instanceBuffer; // 实例数据（每个轨道段的位置和旋转）

    public RailVulkanRenderer(VulkanContext vulkanContext, VulkanTextureManager textureManager, ModConfig config) {
        this.vulkanContext = vulkanContext;
        this.bridge = vulkanContext.getBridge();
        this.textureManager = textureManager;
        this.config = config;
    }

    public void initialize() {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Initializing rail renderer...");

        // 创建大型轨道顶点缓冲区（轨道数据量很大）
        int vertexSizeBytes = config.vertexBufferSizeMB * 1024 * 1024;
        int indexSizeBytes = config.indexBufferSizeMB * 1024 * 1024;

        railVertexBuffer = bridge.createVertexBuffer(vertexSizeBytes);
        railIndexBuffer = bridge.createIndexBuffer(indexSizeBytes);
        instanceBuffer = bridge.createVertexBuffer(1024 * 1024); // 1MB for instances
    }

    public void update(net.minecraft.client.MinecraftClient client) {
        visibleChunks.clear();

        // 获取玩家位置
        double playerX = client.player.getX();
        double playerZ = client.player.getZ();
        int renderDistance = config.lodDistance;

        // 计算可见的区块范围
        int chunkX = (int) Math.floor(playerX) >> 4;
        int chunkZ = (int) Math.floor(playerZ) >> 4;
        int range = renderDistance >> 4;

        for (int dx = -range; dx <= range; dx++) {
            for (int dz = -range; dz <= range; dz++) {
                long chunkKey = encodeChunkKey(chunkX + dx, chunkZ + dz);
                visibleChunks.add(chunkKey);
            }
        }

        // 确保可见区块的轨道网格已上传
        for (Long chunkKey : visibleChunks) {
            ensureChunkRailMesh(chunkKey);
        }
    }

    public void render(float[] vpMatrix, float tickDelta, PlayerEntity player) {
        if (visibleChunks.isEmpty()) return;

        bridge.bindPipeline(vulkanContext.getRailPipeline());

        double camX = player.getX();
        double camY = player.getY();
        double camZ = player.getZ();

        for (Long chunkKey : visibleChunks) {
            String meshId = chunkRailMeshCache.get(chunkKey);
            if (meshId == null) continue;

            int cx = decodeChunkX(chunkKey);
            int cz = decodeChunkZ(chunkKey);

            // 区块的世界坐标偏移
            float offsetX = (float) (cx * 16 - camX);
            float offsetZ = (float) (cz * 16 - camZ);

            // 构建模型矩阵（只有平移）
            float[] modelMatrix = new float[16];
            new Matrix4f().translate(offsetX, 0, offsetZ).get(modelMatrix);

            float[] mvpMatrix = new float[16];
            new Matrix4f(vpMatrix).mul(new Matrix4f(modelMatrix)).get(mvpMatrix);

            bridge.renderMesh(meshId, mvpMatrix, new float[]{1, 1, 1, 1});
        }
    }

    /**
     * 确保指定区块的轨道网格已上传
     */
    private void ensureChunkRailMesh(long chunkKey) {
        chunkRailMeshCache.computeIfAbsent(chunkKey, key -> {
            int cx = decodeChunkX(key);
            int cz = decodeChunkZ(key);

            MeshData mesh = loadChunkRailMesh(cx, cz);
            if (mesh == null || mesh.getVertexCount() == 0) return null;

            String meshId = "rail_" + cx + "_" + cz;
            long texture = textureManager.getOrCreateTexture(
                    new net.minecraft.util.Identifier("mtr", "textures/rail/standard.png"));

            bridge.registerMesh(meshId, mesh.getVertexData(), mesh.getIndexData(), texture);
            return meshId;
        });
    }

    /**
     * 从MTR数据加载指定区块的轨道网格
     */
    private MeshData loadChunkRailMesh(int chunkX, int chunkZ) {
        MeshData mesh = new MeshData();

        try {
            // 通过反射访问MTR的Rail数据
            // MTR的Rail类包含轨道的几何信息
            // 包括：铁轨曲线、枕木位置、连接件等

            // Class<?> railClass = Class.forName("mtr.block.BlockRail");
            // 获取该区块内的所有轨道段
            // List<?> rails = getRailsInChunk(chunkX, chunkZ);
            // for (Object rail : rails) {
            //     generateRailMesh(mesh, rail);
            // }

            // 简化示例：创建一段直线轨道
            generateSampleRailSegment(mesh, 0, 0, 16, 0);

        } catch (Exception e) {
            MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to load rail mesh for chunk ({}, {})", chunkX, chunkZ, e);
        }

        return mesh;
    }

    /**
     * 生成一段轨道网格
     * MTR的轨道使用贝塞尔曲线，这里简化为直线段
     */
    private void generateSampleRailSegment(MeshData mesh, float startX, float startZ,
                                             float endX, float endZ) {
        float railWidth = 1.435f / 2; // 标准轨距的一半
        float railHeight = 0.1f;
        float sleeperWidth = 2.5f;
        float sleeperHeight = 0.15f;
        float sleeperSpacing = 0.6f;

        float dx = endX - startX;
        float dz = endZ - startZ;
        float length = (float) Math.sqrt(dx * dx + dz * dz);
        float nx = -dz / length; // 法线方向
        float nz = dx / length;

        // 生成两条铁轨
        for (int rail = 0; rail < 2; rail++) {
            float sign = rail == 0 ? 1 : -1;
            float ox = nx * railWidth * sign;
            float oz = nz * railWidth * sign;

            int base = mesh.getVertexCount();
            // 铁轨截面（简化的工字形）
            mesh.addVertex(startX + ox - 0.03f, railHeight, startZ + oz - 0.03f,
                    0, 1, 0, 0, 0, 0.6f, 0.6f, 0.6f, 1);
            mesh.addVertex(startX + ox + 0.03f, railHeight, startZ + oz + 0.03f,
                    0, 1, 0, 1, 0, 0.6f, 0.6f, 0.6f, 1);
            mesh.addVertex(endX + ox + 0.03f, railHeight, endZ + oz + 0.03f,
                    0, 1, 0, 1, length, 0.6f, 0.6f, 0.6f, 1);
            mesh.addVertex(endX + ox - 0.03f, railHeight, endZ + oz - 0.03f,
                    0, 1, 0, 0, length, 0.6f, 0.6f, 0.6f, 1);
            mesh.addQuad(base, base + 1, base + 2, base + 3);
        }

        // 生成枕木
        int sleeperCount = (int) (length / sleeperSpacing);
        for (int i = 0; i < sleeperCount; i++) {
            float t = (float) i / sleeperCount;
            float sx = startX + dx * t;
            float sz = startZ + dz * t;

            int base = mesh.getVertexCount();
            // 枕木（长方体）
            float hw = sleeperWidth / 2;
            mesh.addVertex(sx - nx * hw, 0, sz - nz * hw,
                    0, 1, 0, 0, 0, 0.4f, 0.3f, 0.2f, 1);
            mesh.addVertex(sx + nx * hw, 0, sz + nz * hw,
                    0, 1, 0, 1, 0, 0.4f, 0.3f, 0.2f, 1);
            mesh.addVertex(sx + nx * hw, sleeperHeight, sz + nz * hw,
                    0, 1, 0, 1, 1, 0.4f, 0.3f, 0.2f, 1);
            mesh.addVertex(sx - nx * hw, sleeperHeight, sz - nz * hw,
                    0, 1, 0, 0, 1, 0.4f, 0.3f, 0.2f, 1);
            mesh.addQuad(base, base + 1, base + 2, base + 3);
        }
    }

    private static long encodeChunkKey(int cx, int cz) {
        return ((long) cx << 32) | (cz & 0xFFFFFFFFL);
    }

    private static int decodeChunkX(long key) {
        return (int) (key >> 32);
    }

    private static int decodeChunkZ(long key) {
        return (int) key;
    }

    public void cleanup() {
        for (String meshId : chunkRailMeshCache.values()) {
            bridge.unregisterMesh(meshId);
        }
        chunkRailMeshCache.clear();
        if (railVertexBuffer != 0) bridge.destroyBuffer(railVertexBuffer);
        if (railIndexBuffer != 0) bridge.destroyBuffer(railIndexBuffer);
        if (instanceBuffer != 0) bridge.destroyBuffer(instanceBuffer);
    }
}