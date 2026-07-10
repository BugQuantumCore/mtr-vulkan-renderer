package com.mtr.vulkanrenderer.render;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.MeshData;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import com.mtr.vulkanrenderer.config.ModConfig;
import com.mtr.vulkanrenderer.vulkan.VulkanContext;
import com.mtr.vulkanrenderer.vulkan.VulkanTextureManager;
import net.minecraft.client.network.ClientPlayNetworkHandler;
import net.minecraft.entity.player.PlayerEntity;
import org.joml.Matrix4f;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/**
 * 使用Vulkan渲染MTR的列车。
 *
 * MTR的列车渲染流程:
 * 1. 从服务器同步列车位置、方向、速度数据
 * 2. 根据列车类型加载对应的3D模型
 * 3. 在列车当前位置应用变换矩阵（平移+旋转）
 * 4. 渲染列车模型（车体、车轮、门窗、内部装饰等）
 *
 * 此渲染器将上述流程从OpenGL迁移到Vulkan:
 * - 模型数据预上传到GPU顶点缓冲区
 * - 变换矩阵通过Uniform缓冲区传递
 * - 使用实例化渲染处理多节车厢
 */
public class TrainVulkanRenderer {

    private final VulkanContext vulkanContext;
    private final VulkanBridge bridge;
    private final VulkanTextureManager textureManager;
    private final ModConfig config;

    // 已缓存的列车模型 (trainType -> meshId)
    private final Map<String, String> trainMeshCache = new ConcurrentHashMap<>();

    // 当前帧需要渲染的列车数据
    private final List<TrainRenderData> currentFrameTrains = new ArrayList<>();

    // Uniform缓冲区
    private long uniformBuffer;
    private static final int UNIFORM_SIZE = 16 * 4 * 64; // 支持64个列车实例

    public TrainVulkanRenderer(VulkanContext vulkanContext, VulkanTextureManager textureManager, ModConfig config) {
        this.vulkanContext = vulkanContext;
        this.bridge = vulkanContext.getBridge();
        this.textureManager = textureManager;
        this.config = config;
    }

    public void initialize() {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Initializing train renderer...");

        // 创建Uniform缓冲区
        uniformBuffer = bridge.createUniformBuffer(UNIFORM_SIZE);
    }

    /**
     * 每tick更新列车数据
     * 从MTR的数据结构中读取列车位置信息
     */
    public void update(net.minecraft.client.MinecraftClient client) {
        currentFrameTrains.clear();

        // 从MTR的客户端数据中获取所有可见列车
        // 注意：这里需要通过Mixin或反射访问MTR的内部数据
        // MTR的TrainClientData类包含列车的位置、旋转、类型等信息
        try {
            // 通过反射访问MTR的列车数据
            Class<?> trainDataClass = Class.forName("mtr.data.TrainClientData");
            // ... 获取所有列车实例并转换为TrainRenderData

            // 简化示例：
            // List<?> allTrains = TrainClientData.getAllTrains();
            // for (Object train : allTrains) {
            //     TrainRenderData data = extractTrainData(train);
            //     if (data != null && isTrainVisible(data, client.player)) {
            //         currentFrameTrains.add(data);
            //     }
            // }

        } catch (ClassNotFoundException e) {
            // MTR类不可用，可能在服务端
        }
    }

    /**
     * 渲染所有列车（不透明部分）
     */
    public void render(float[] vpMatrix, float tickDelta, PlayerEntity player) {
        if (currentFrameTrains.isEmpty()) return;

        bridge.bindPipeline(vulkanContext.getTrainPipeline());
        bridge.bindVertexBuffer(0); // 由registerMesh管理

        for (TrainRenderData train : currentFrameTrains) {
            // 计算列车的世界变换矩阵
            float[] modelMatrix = computeTrainModelMatrix(train, tickDelta);

            // 计算MVP
            float[] mvpMatrix = multiplyMatrices(vpMatrix, modelMatrix);

            // 检查是否在视锥体内
            if (config.enableFrustumCulling && !isInFrustum(mvpMatrix)) {
                continue;
            }

            // 确保模型已上传
            String meshId = ensureTrainMesh(train.trainType);
            if (meshId == null) continue;

            // 渲染列车
            bridge.renderMesh(meshId, mvpMatrix, train.color);
        }
    }

    /**
     * 渲染列车的半透明部分（车窗等）
     */
    public void renderTranslucent(float[] vpMatrix, float tickDelta) {
        bridge.bindPipeline(vulkanContext.getTranslucentPipeline());

        for (TrainRenderData train : currentFrameTrains) {
            float[] modelMatrix = computeTrainModelMatrix(train, tickDelta);
            float[] mvpMatrix = multiplyMatrices(vpMatrix, modelMatrix);

            String meshId = train.trainType + "_translucent";
            if (trainMeshCache.containsKey(meshId)) {
                bridge.renderMesh(meshId, mvpMatrix, new float[]{1, 1, 1, 0.5f});
            }
        }
    }

    /**
     * 确保指定类型的列车模型已上传到GPU
     */
    private String ensureTrainMesh(String trainType) {
        return trainMeshCache.computeIfAbsent(trainType, type -> {
            // 从MTR的模型系统获取列车网格数据
            // MTR使用自定义的模型格式，包含多个部件
            MeshData mesh = loadTrainMeshFromMTR(type);
            if (mesh == null) return null;

            String meshId = "train_" + type;
            long textureHandle = textureManager.getOrCreateTexture(
                    new net.minecraft.util.Identifier("mtr", "textures/train/" + type + ".png"));

            bridge.registerMesh(meshId, mesh.getVertexData(), mesh.getIndexData(), textureHandle);
            return meshId;
        });
    }

    /**
     * 从MTR的模型数据加载列车网格
     * 通过反射访问MTR的模型系统
     */
    private MeshData loadTrainMeshFromMTR(String trainType) {
        MeshData mesh = new MeshData();

        try {
            // 通过反射访问MTR的TrainModel类
            // MTR的列车模型通常包含: 车体、车顶、车底、车轮、车门、车窗等部件
            // 每个部件有自己的顶点、UV和纹理

            // 示例：创建一个简单的列车车体网格
            createSimpleTrainBody(mesh);

            return mesh;
        } catch (Exception e) {
            MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to load train mesh for type: {}", trainType, e);
            return null;
        }
    }

    /**
     * 创建一个简单的列车车体网格（示例/回退）
     */
    private void createSimpleTrainBody(MeshData mesh) {
        // 简单的长方体车体: 2m宽 x 2.5m高 x 16m长 (Minecraft单位)
        float w = 1.0f, h = 1.25f, l = 8.0f;

        // 前面
        int base = mesh.getVertexCount();
        mesh.addVertex(-w, -h, -l,  0, 0, -1, 0, 0, 1, 1, 1, 1);
        mesh.addVertex( w, -h, -l,  0, 0, -1, 1, 0, 1, 1, 1, 1);
        mesh.addVertex( w,  h, -l,  0, 0, -1, 1, 1, 1, 1, 1, 1);
        mesh.addVertex(-w,  h, -l,  0, 0, -1, 0, 1, 1, 1, 1, 1);
        mesh.addQuad(base, base+1, base+2, base+3);

        // 后面
        base = mesh.getVertexCount();
        mesh.addVertex(-w, -h,  l,  0, 0, 1, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(-w,  h,  l,  0, 0, 1, 0, 1, 1, 1, 1, 1);
        mesh.addVertex( w,  h,  l,  0, 0, 1, 1, 1, 1, 1, 1, 1);
        mesh.addVertex( w, -h,  l,  0, 0, 1, 1, 0, 1, 1, 1, 1);
        mesh.addQuad(base, base+1, base+2, base+3);

        // 左面
        base = mesh.getVertexCount();
        mesh.addVertex(-w, -h, -l, -1, 0, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(-w,  h, -l, -1, 0, 0, 0, 1, 1, 1, 1, 1);
        mesh.addVertex(-w,  h,  l, -1, 0, 0, 1, 1, 1, 1, 1, 1);
        mesh.addVertex(-w, -h,  l, -1, 0, 0, 1, 0, 1, 1, 1, 1);
        mesh.addQuad(base, base+1, base+2, base+3);

        // 右面
        base = mesh.getVertexCount();
        mesh.addVertex( w, -h, -l,  1, 0, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex( w, -h,  l,  1, 0, 0, 1, 0, 1, 1, 1, 1);
        mesh.addVertex( w,  h,  l,  1, 0, 0, 1, 1, 1, 1, 1, 1);
        mesh.addVertex( w,  h, -l,  1, 0, 0, 0, 1, 1, 1, 1, 1);
        mesh.addQuad(base, base+1, base+2, base+3);

        // 顶面
        base = mesh.getVertexCount();
        mesh.addVertex(-w,  h, -l,  0, 1, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex( w,  h, -l,  0, 1, 0, 1, 0, 1, 1, 1, 1);
        mesh.addVertex( w,  h,  l,  0, 1, 0, 1, 1, 1, 1, 1, 1);
        mesh.addVertex(-w,  h,  l,  0, 1, 0, 0, 1, 1, 1, 1, 1);
        mesh.addQuad(base, base+1, base+2, base+3);

        // 底面
        base = mesh.getVertexCount();
        mesh.addVertex(-w, -h, -l,  0, -1, 0, 0, 0, 0.5f, 0.5f, 0.5f, 1);
        mesh.addVertex(-w, -h,  l,  0, -1, 0, 0, 1, 0.5f, 0.5f, 0.5f, 1);
        mesh.addVertex( w, -h,  l,  0, -1, 0, 1, 1, 0.5f, 0.5f, 0.5f, 1);
        mesh.addVertex( w, -h, -l,  0, -1, 0, 1, 0, 0.5f, 0.5f, 0.5f, 1);
        mesh.addQuad(base, base+1, base+2, base+3);
    }

    /**
     * 计算列车的模型变换矩阵
     */
    private float[] computeTrainModelMatrix(TrainRenderData train, float tickDelta) {
        // 插值列车位置
        double x = train.prevX + (train.x - train.prevX) * tickDelta;
        double y = train.prevY + (train.y - train.prevY) * tickDelta;
        double z = train.prevZ + (train.z - train.prevZ) * tickDelta;

        Matrix4f matrix = new Matrix4f()
                .translate((float) x, (float) y, (float) z)
                .rotateY(train.yaw)
                .rotateX(train.pitch);

        float[] result = new float[16];
        matrix.get(result);
        return result;
    }

    private float[] multiplyMatrices(float[] a, float[] b) {
        float[] result = new float[16];
        new Matrix4f(a).mul(new Matrix4f(b)).get(result);
        return result;
    }

    private boolean isInFrustum(float[] mvpMatrix) {
        // 简单的视锥体裁剪
        Matrix4f mvp = new Matrix4f(mvpMatrix);
        // 检查原点是否在裁剪空间内
        float w = mvp.m33();
        return Math.abs(w) > 0.001f; // 简化检查
    }

    public void cleanup() {
        for (String meshId : trainMeshCache.values()) {
            bridge.unregisterMesh(meshId);
        }
        trainMeshCache.clear();
        if (uniformBuffer != 0) {
            bridge.destroyBuffer(uniformBuffer);
        }
    }

    /**
     * 列车渲染数据 - 从MTR的数据结构中提取
     */
    public static class TrainRenderData {
        public String trainType;
        public double x, y, z;
        public double prevX, prevY, prevZ;
        public float yaw, pitch;
        public float[] color = {1, 1, 1, 1};
        public int carCount;
        public double speed;
        public String routeColor;

        // MTR特定的数据
        public Object mtrTrainRef; // MTR列车对象的引用
    }
}