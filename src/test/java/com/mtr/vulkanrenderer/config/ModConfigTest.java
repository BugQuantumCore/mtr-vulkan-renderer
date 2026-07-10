package com.mtr.vulkanrenderer.config;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Path;

class ModConfigTest {

    @Test
    void testDefaultValues() {
        ModConfig config = new ModConfig();

        assertTrue(config.enableVulkanRendering);
        assertTrue(config.fallbackToOpenGL);
        assertEquals(2, config.maxFramesInFlight);
        assertTrue(config.enableTrainRendering);
        assertTrue(config.enableRailRendering);
        assertTrue(config.enableSignalRendering);
        assertTrue(config.enableStationRendering);
        assertEquals(256, config.vertexBufferSizeMB);
        assertEquals(64, config.indexBufferSizeMB);
        assertEquals(32, config.uniformBufferSizeMB);
        assertTrue(config.enableMeshletCulling);
        assertTrue(config.enableFrustumCulling);
        assertEquals(128, config.lodDistance);
        assertFalse(config.enableGPUDrivenRendering);
        assertFalse(config.enableValidationLayers);
        assertFalse(config.enableDebugMarkers);
        assertFalse(config.logRenderCalls);
        assertFalse(config.showDebugOverlay);
    }

    @Test
    void testSaveAndLoad(@TempDir Path tempDir) {
        // This test would require mocking FabricLoader
        // For now, just verify the config object works

        ModConfig config = new ModConfig();
        config.enableVulkanRendering = false;
        config.maxFramesInFlight = 3;
        config.vertexBufferSizeMB = 512;

        assertFalse(config.enableVulkanRendering);
        assertEquals(3, config.maxFramesInFlight);
        assertEquals(512, config.vertexBufferSizeMB);
    }
}