package com.mtr.vulkanrenderer.render;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import com.mtr.vulkanrenderer.config.ModConfig;
import com.mtr.vulkanrenderer.vulkan.VulkanContext;
import com.mtr.vulkanrenderer.vulkan.VulkanTextureManager;

/**
 * 使用Vulkan渲染MTR的车站设施
 * 包括：站台屏蔽门(PSD)、电梯、扶梯、售票机、闸机、指示牌等
 */
public class StationVulkanRenderer {

    private final VulkanContext vulkanContext;
    private final VulkanBridge bridge;
    private final VulkanTextureManager textureManager;
    private final ModConfig config;

    public StationVulkanRenderer(VulkanContext vulkanContext, VulkanTextureManager textureManager, ModConfig config) {
        this.vulkanContext = vulkanContext;
        this.bridge = vulkanContext.getBridge();
        this.textureManager = textureManager;
        this.config = config;
    }

    public void initialize() {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Initializing station renderer...");
        // 预创建车站设施的网格：PSD、电梯、扶梯等
    }

    public void render(float[] vpMatrix, float tickDelta) {
        bridge.bindPipeline(vulkanContext.getStationPipeline());
        // 渲染所有可见的车站设施
        // MTR的车站设施包括：
        // - Platform Screen Doors (PSD) - 站台屏蔽门
        // - Escalators - 扶梯
        // - Elevators - 电梯
        // - Ticket barriers - 闸机
        // - Route signs - 线路指示牌
        // - Station name boards - 站名牌
    }

    public void cleanup() {
        // 清理所有车站设施网格
    }
}