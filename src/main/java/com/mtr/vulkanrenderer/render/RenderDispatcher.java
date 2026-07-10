package com.mtr.vulkanrenderer.render;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import com.mtr.vulkanrenderer.config.ModConfig;
import com.mtr.vulkanrenderer.vulkan.VulkanContext;
import com.mtr.vulkanrenderer.vulkan.VulkanTextureManager;
import net.fabricmc.fabric.api.client.rendering.v1.WorldRenderContext;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.util.math.MatrixStack;
import org.joml.Matrix4f;

import java.util.ArrayList;
import java.util.List;

/**
 * 渲染调度器 - 管理所有MTR实体的Vulkan渲染
 * 在每个渲染阶段协调列车、轨道、信号等的渲染
 */
public class RenderDispatcher {

    private final VulkanContext vulkanContext;
    private final VulkanBridge bridge;
    private final ModConfig config;
    private final VulkanTextureManager textureManager;

    private TrainVulkanRenderer trainRenderer;
    private RailVulkanRenderer railRenderer;
    private SignalVulkanRenderer signalRenderer;
    private StationVulkanRenderer stationRenderer;

    // 渲染统计
    private int frameCount = 0;
    private int totalDrawCalls = 0;
    private int totalTriangles = 0;
    private float lastFrameTime = 0;

    // MVP矩阵缓存
    private final float[] viewMatrix = new float[16];
    private final float[] projectionMatrix = new float[16];
    private final float[] vpMatrix = new float[16];

    public RenderDispatcher(VulkanContext vulkanContext, VulkanBridge bridge, ModConfig config) {
        this.vulkanContext = vulkanContext;
        this.bridge = bridge;
        this.config = config;
        this.textureManager = new VulkanTextureManager(bridge);
    }

    public void initialize() {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Initializing render subsystems...");

        if (config.enableTrainRendering) {
            trainRenderer = new TrainVulkanRenderer(vulkanContext, textureManager, config);
            trainRenderer.initialize();
        }

        if (config.enableRailRendering) {
            railRenderer = new RailVulkanRenderer(vulkanContext, textureManager, config);
            railRenderer.initialize();
        }

        if (config.enableSignalRendering) {
            signalRenderer = new SignalVulkanRenderer(vulkanContext, textureManager, config);
            signalRenderer.initialize();
        }

        if (config.enableStationRendering) {
            stationRenderer = new StationVulkanRenderer(vulkanContext, textureManager, config);
            stationRenderer.initialize();
        }

        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] All render subsystems initialized");
    }

    /**
     * 在实体渲染之前调用 - 更新相机和矩阵
     */
    public void onBeforeEntities(WorldRenderContext context) {
        long startTime = System.nanoTime();

        // 开始帧
        bridge.beginFrame();

        // 更新视图和投影矩阵
        MatrixStack matrices = context.matrixStack();
        if (matrices != null) {
            Matrix4f view = context.camera().getPositionMatrix(new Matrix4f());
            Matrix4f proj = context.projectionMatrix().peekLast();

            view.get(viewMatrix);
            proj.get(projectionMatrix);

            // 计算VP矩阵
            new Matrix4f(proj).mul(view).get(vpMatrix);
        }

        frameCount++;
    }

    /**
     * 在实体渲染之后调用 - 渲染MTR的不透明内容
     */
    public void onAfterEntities(WorldRenderContext context) {
        MinecraftClient client = MinecraftClient.getInstance();
        float tickDelta = context.tickDelta();

        if (config.enableDebugMarkers) {
            bridge.debugMarker("MTR Opaque Pass", 0.2f, 0.6f, 1.0f);
        }

        // 渲染列车（不透明部分）
        if (trainRenderer != null) {
            trainRenderer.render(vpMatrix, tickDelta, client.player);
        }

        // 渲染轨道
        if (railRenderer != null) {
            railRenderer.render(vpMatrix, tickDelta, client.player);
        }

        // 渲染信号
        if (signalRenderer != null) {
            signalRenderer.render(vpMatrix, tickDelta);
        }

        // 渲染车站装饰
        if (stationRenderer != null) {
            stationRenderer.render(vpMatrix, tickDelta);
        }
    }

    /**
     * 在半透明渲染阶段调用 - 渲染MTR的半透明内容
     */
    public void onAfterTranslucent(WorldRenderContext context) {
        if (config.enableDebugMarkers) {
            bridge.debugMarker("MTR Translucent Pass", 1.0f, 0.6f, 0.2f);
        }

        // 渲染列车的半透明部分（车窗等）
        if (trainRenderer != null) {
            trainRenderer.renderTranslucent(vpMatrix, context.tickDelta());
        }

        // 渲染信号的半透明部分（灯光光晕）
        if (signalRenderer != null) {
            signalRenderer.renderTranslucent(vpMatrix, context.tickDelta());
        }

        // 结束帧
        bridge.endFrame();
        bridge.waitForFrame();

        // 统计
        float[] stats = bridge.getRenderStats();
        if (stats != null && stats.length >= 3) {
            totalDrawCalls += (int) stats[0];
            totalTriangles += (int) stats[1];
            lastFrameTime = stats[2];
        }
    }

    /**
     * 每tick调用 - 更新动态数据
     */
    public void onTick(MinecraftClient client) {
        if (trainRenderer != null) {
            trainRenderer.update(client);
        }
        if (railRenderer != null) {
            railRenderer.update(client);
        }
    }

    public void cleanup() {
        if (trainRenderer != null) trainRenderer.cleanup();
        if (railRenderer != null) railRenderer.cleanup();
        if (signalRenderer != null) signalRenderer.cleanup();
        if (stationRenderer != null) stationRenderer.cleanup();
        textureManager.cleanup();
    }

    public int getFrameCount() { return frameCount; }
    public float getLastFrameTime() { return lastFrameTime; }
}