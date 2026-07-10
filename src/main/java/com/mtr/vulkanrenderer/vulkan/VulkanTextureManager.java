package com.mtr.vulkanrenderer.vulkan;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.texture.NativeImage;
import net.minecraft.util.Identifier;

import java.util.HashMap;
import java.util.Map;

/**
 * 管理MTR相关的Vulkan纹理
 * 负责将Minecraft的纹理转换为Vulkan纹理
 */
public class VulkanTextureManager {

    private final VulkanBridge bridge;
    private final Map<Identifier, Long> textureCache = new HashMap<>();
    private final Map<String, Long> meshTextures = new HashMap<>();

    public VulkanTextureManager(VulkanBridge bridge) {
        this.bridge = bridge;
    }

    /**
     * 从Minecraft的Identifier获取Vulkan纹理
     */
    public long getOrCreateTexture(Identifier textureId) {
        return textureCache.computeIfAbsent(textureId, id -> {
            try {
                MinecraftClient client = MinecraftClient.getInstance();
                // 获取纹理的NativeImage
                var textureManager = client.getTextureManager();
                var abstractTexture = textureManager.getTexture(id);

                // 获取纹理的GL ID和尺寸
                int glId = abstractTexture.getGlId();
                // 使用VulkanMod的GL互操作来获取纹理数据
                // 这里简化处理 - 实际需要从GL纹理读取像素数据
                return bridge.createTextureFromGL(glId, 256, 256);
            } catch (Exception e) {
                MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to create texture for {}", id, e);
                return 0L;
            }
        });
    }

    /**
     * 从原始像素数据创建纹理
     */
    public long createTextureFromPixels(byte[] rgbaPixels, int width, int height) {
        return bridge.createTexture(rgbaPixels, width, height, 1);
    }

    /**
     * 缓存网格关联的纹理
     */
    public void registerMeshTexture(String meshId, long textureHandle) {
        meshTextures.put(meshId, textureHandle);
    }

    /**
     * 获取网格关联的纹理
     */
    public long getMeshTexture(String meshId) {
        return meshTextures.getOrDefault(meshId, 0L);
    }

    public void cleanup() {
        textureCache.values().forEach(bridge::destroyTexture);
        textureCache.clear();
        meshTextures.values().forEach(bridge::destroyTexture);
        meshTextures.clear();
    }
}