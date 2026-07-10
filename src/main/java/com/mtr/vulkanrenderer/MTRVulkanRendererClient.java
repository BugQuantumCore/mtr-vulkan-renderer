package com.mtr.vulkanrenderer;

import com.mtr.vulkanrenderer.bridge.NativeLibraryLoader;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import com.mtr.vulkanrenderer.config.ModConfig;
import com.mtr.vulkanrenderer.render.RenderDispatcher;
import com.mtr.vulkanrenderer.vulkan.VulkanContext;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.fabric.api.client.rendering.v1.WorldRenderEvents;
import net.minecraft.client.MinecraftClient;

public class MTRVulkanRendererClient implements ClientModInitializer {

    private static MTRVulkanRendererClient instance;
    private VulkanContext vulkanContext;
    private RenderDispatcher renderDispatcher;
    private VulkanBridge vulkanBridge;
    private ModConfig config;
    private boolean initialized = false;
    private boolean vulkanAvailable = false;

    public static MTRVulkanRendererClient getInstance() {
        return instance;
    }

    @Override
    public void onInitializeClient() {
        instance = this;
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Client-side initialization starting...");

        // 加载配置
        config = ModConfig.load();

        // 注册生命周期事件
        ClientLifecycleEvents.CLIENT_STARTED.register(this::onClientStarted);
        ClientLifecycleEvents.CLIENT_STOPPING.register(this::onClientStopping);

        // 注册渲染事件
        WorldRenderEvents.BEFORE_ENTITIES.register(context -> {
            if (initialized && vulkanAvailable) {
                renderDispatcher.onBeforeEntities(context);
            }
        });

        WorldRenderEvents.AFTER_ENTITIES.register(context -> {
            if (initialized && vulkanAvailable) {
                renderDispatcher.onAfterEntities(context);
            }
        });

        WorldRenderEvents.AFTER_TRANSLUCENT.register(context -> {
            if (initialized && vulkanAvailable) {
                renderDispatcher.onAfterTranslucent(context);
            }
        });

        // 注册tick事件用于更新渲染数据
        ClientTickEvents.END_CLIENT_TICK.register(client -> {
            if (initialized && vulkanAvailable) {
                renderDispatcher.onTick(client);
            }
        });

        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Client-side initialization complete");
    }

    private void onClientStarted(MinecraftClient client) {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Minecraft client started, initializing Vulkan subsystem...");

        try {
            // 1. 加载C++原生库
            NativeLibraryLoader.load();
            MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Native C++ library loaded successfully");

            // 2. 初始化Vulkan桥接
            vulkanBridge = new VulkanBridge();

            // 3. 检查Vulkan可用性 (依赖VulkanMod提供的Vulkan实例)
            vulkanAvailable = vulkanBridge.initializeVulkan();
            if (!vulkanAvailable) {
                MTRVulkanRenderer.LOGGER.warn("[MTR-Vulkan] Vulkan not available, falling back to OpenGL");
                return;
            }

            // 4. 创建Vulkan上下文
            vulkanContext = new VulkanContext(vulkanBridge);
            vulkanContext.initialize();

            // 5. 创建渲染调度器
            renderDispatcher = new RenderDispatcher(vulkanContext, vulkanBridge, config);
            renderDispatcher.initialize();

            initialized = true;
            MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Vulkan rendering subsystem fully initialized");

        } catch (Exception e) {
            MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to initialize Vulkan rendering", e);
            vulkanAvailable = false;
        }
    }

    private void onClientStopping(MinecraftClient client) {
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Cleaning up Vulkan resources...");

        if (renderDispatcher != null) {
            renderDispatcher.cleanup();
        }
        if (vulkanContext != null) {
            vulkanContext.cleanup();
        }
        if (vulkanBridge != null) {
            vulkanBridge.cleanup();
        }

        initialized = false;
        MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Vulkan resources cleaned up");
    }

    public boolean isVulkanAvailable() {
        return vulkanAvailable && initialized;
    }

    public RenderDispatcher getRenderDispatcher() {
        return renderDispatcher;
    }

    public VulkanContext getVulkanContext() {
        return vulkanContext;
    }

    public ModConfig getConfig() {
        return config;
    }
}