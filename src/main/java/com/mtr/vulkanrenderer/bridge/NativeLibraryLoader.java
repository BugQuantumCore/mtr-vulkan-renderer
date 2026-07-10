package com.mtr.vulkanrenderer.bridge;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import net.fabricmc.loader.api.FabricLoader;
import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

public class NativeLibraryLoader {

    private static boolean loaded = false;
    private static final String LIB_NAME = "mtr_vulkan_renderer";

    public static void load() throws UnsatisfiedLinkError {
        if (loaded) return;

        String osName = System.getProperty("os.name").toLowerCase();
        String libFileName;
        if (osName.contains("win")) {
            libFileName = LIB_NAME + ".dll";
        } else if (osName.contains("mac") || osName.contains("darwin")) {
            libFileName = "lib" + LIB_NAME + ".dylib";
        } else {
            libFileName = "lib" + LIB_NAME + ".so";
        }

        try {
            // 尝试从模组JAR中提取原生库
            Path tempDir = FabricLoader.getInstance().getGameDir()
                    .resolve("mtr-vulkan-renderer-native");
            Files.createDirectories(tempDir);
            Path libPath = tempDir.resolve(libFileName);

            // 从classpath中查找
            String resourcePath = "/native/" + libFileName;
            try (InputStream is = NativeLibraryLoader.class.getResourceAsStream(resourcePath)) {
                if (is != null) {
                    Files.copy(is, libPath, StandardCopyOption.REPLACE_EXISTING);
                    MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Extracted native library to {}", libPath);
                }
            }

            // 如果提取成功，从提取位置加载
            if (Files.exists(libPath)) {
                System.load(libPath.toAbsolutePath().toString());
                loaded = true;
                MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Native library loaded from {}", libPath);
                return;
            }

            // 回退：尝试从系统库路径加载
            System.loadLibrary(LIB_NAME);
            loaded = true;
            MTRVulkanRenderer.LOGGER.info("[MTR-Vulkan] Native library loaded from system path");

        } catch (IOException e) {
            throw new UnsatisfiedLinkError(
                    "[MTR-Vulkan] Failed to load native library: " + e.getMessage());
        }
    }

    public static boolean isLoaded() {
        return loaded;
    }
}