package com.mtr.vulkanrenderer;

import net.fabricmc.api.ModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MTRVulkanRenderer implements ModInitializer {

    public static final String MOD_ID = "mtr-vulkan-renderer";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    @Override
    public void onInitialize() {
        LOGGER.info("[MTR-Vulkan] MTR Vulkan Renderer initialized");
        LOGGER.info("[MTR-Vulkan] This mod replaces MTR's OpenGL renderer with Vulkan via C++ native code");
    }
}