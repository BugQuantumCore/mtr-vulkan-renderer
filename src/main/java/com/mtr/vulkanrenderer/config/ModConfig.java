package com.mtr.vulkanrenderer.config;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import net.fabricmc.loader.api.FabricLoader;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

public class ModConfig {

    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();
    private static final Path CONFIG_PATH = FabricLoader.getInstance()
            .getConfigDir().resolve("mtr-vulkan-renderer.json");

    // 渲染选项
    public boolean enableVulkanRendering = true;
    public boolean fallbackToOpenGL = true;
    public int maxFramesInFlight = 2;
    public boolean enableTrainRendering = true;
    public boolean enableRailRendering = true;
    public boolean enableSignalRendering = true;
    public boolean enableStationRendering = true;

    // 性能选项
    public int vertexBufferSizeMB = 256;
    public int indexBufferSizeMB = 64;
    public int uniformBufferSizeMB = 32;
    public boolean enableMeshletCulling = true;
    public boolean enableFrustumCulling = true;
    public int lodDistance = 128;
    public boolean enableGPUDrivenRendering = false;

    // 调试选项
    public boolean enableValidationLayers = false;
    public boolean enableDebugMarkers = false;
    public boolean logRenderCalls = false;
    public boolean showDebugOverlay = false;

    public static ModConfig load() {
        if (Files.exists(CONFIG_PATH)) {
            try {
                String json = Files.readString(CONFIG_PATH);
                ModConfig config = GSON.fromJson(json, ModConfig.class);
                MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Configuration loaded from {}", CONFIG_PATH);
                return config;
            } catch (IOException e) {
                MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to load config, using defaults", e);
            }
        }
        ModConfig config = new ModConfig();
        config.save();
        return config;
    }

    public void save() {
        try {
            Files.createDirectories(CONFIG_PATH.getParent());
            Files.writeString(CONFIG_PATH, GSON.toJson(this));
        } catch (IOException e) {
            MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to save config", e);
        }
    }
}