package com.mtr.vulkanrenderer.vulkan;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;

/**
 * 管理Vulkan渲染上下文。
 * 实际的Vulkan实例和设备由VulkanMod创建和管理，
 * 我们复用其Vulkan资源来渲染MTR的内容。
 */
public class VulkanContext {

    private final VulkanBridge bridge;
    private boolean initialized = false;
    private String gpuName;
    private long totalGPUMemoryMB;

    // 共享的管线
    private long trainPipeline;
    private long railPipeline;
    private long signalPipeline;
    private long stationPipeline;
    private long translucentPipeline;

    public VulkanContext(VulkanBridge bridge) {
        this.bridge = bridge;
    }

    public void initialize() {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Creating Vulkan pipelines...");

        // 获取GPU信息
        gpuName = bridge.getGPUInfo();
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] GPU: {}", gpuName);

        // 创建列车渲染管线
        // 顶点格式: pos(3f) + normal(3f) + uv(2f) + color(4f) = 12 floats
        trainPipeline = bridge.createPipeline(
                "shaders/train.vert.spv",
                "shaders/train.frag.spv",
                "3f,3f,2f,4f",  // position, normal, uv, color
                0,               // triangle list
                false,           // no blend (opaque)
                true             // depth test
        );

        // 创建轨道渲染管线
        railPipeline = bridge.createPipeline(
                "shaders/rail.vert.spv",
                "shaders/rail.frag.spv",
                "3f,3f,2f,4f",
                0,
                false,
                true
        );

        // 创建信号渲染管线（需要透明度混合）
        signalPipeline = bridge.createPipeline(
                "shaders/signal.vert.spv",
                "shaders/signal.frag.spv",
                "3f,3f,2f,4f",
                0,
                true,            // blend enabled for signal lights
                true
        );

        // 创建车站装饰渲染管线
        stationPipeline = bridge.createPipeline(
                "shaders/station.vert.spv",
                "shaders/station.frag.spv",
                "3f,3f,2f,4f",
                0,
                false,
                true
        );

        // 创建半透明渲染管线（用于车窗等）
        translucentPipeline = bridge.createPipeline(
                "shaders/translucent.vert.spv",
                "shaders/translucent.frag.spv",
                "3f,3f,2f,4f",
                0,
                true,
                true
        );

        initialized = true;
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] All pipelines created successfully");
    }

    public void cleanup() {
        if (!initialized) return;

        bridge.destroyPipeline(trainPipeline);
        bridge.destroyPipeline(railPipeline);
        bridge.destroyPipeline(signalPipeline);
        bridge.destroyPipeline(stationPipeline);
        bridge.destroyPipeline(translucentPipeline);

        initialized = false;
    }

    public long getTrainPipeline() { return trainPipeline; }
    public long getRailPipeline() { return railPipeline; }
    public long getSignalPipeline() { return signalPipeline; }
    public long getStationPipeline() { return stationPipeline; }
    public long getTranslucentPipeline() { return translucentPipeline; }
    public VulkanBridge getBridge() { return bridge; }
    public String getGpuName() { return gpuName; }
    public boolean isInitialized() { return initialized; }
}