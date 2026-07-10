package com.mtr.vulkanrenderer.render;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.bridge.MeshData;
import com.mtr.vulkanrenderer.bridge.VulkanBridge;
import com.mtr.vulkanrenderer.config.ModConfig;
import com.mtr.vulkanrenderer.vulkan.VulkanContext;
import com.mtr.vulkanrenderer.vulkan.VulkanTextureManager;
import org.joml.Matrix4f;

import java.util.*;

/**
 * 使用Vulkan渲染MTR的信号灯
 */
public class SignalVulkanRenderer {

    private final VulkanContext vulkanContext;
    private final VulkanBridge bridge;
    private final VulkanTextureManager textureManager;
    private final ModConfig config;

    private long signalMeshId;
    private final List<SignalRenderData> visibleSignals = new ArrayList<>();

    public SignalVulkanRenderer(VulkanContext vulkanContext, VulkanTextureManager textureManager, ModConfig config) {
        this.vulkanContext = vulkanContext;
        this.bridge = vulkanContext.getBridge();
        this.textureManager = textureManager;
        this.config = config;
    }

    public void initialize() {
        // 创建信号灯的基础网格
        MeshData mesh = createSignalMesh();
        long texture = textureManager.getOrCreateTexture(
                new net.minecraft.util.Identifier("mtr", "textures/signal/signal.png"));
        bridge.registerMesh("signal_base", mesh.getVertexData(), mesh.getIndexData(), texture);
    }

    public void render(float[] vpMatrix, float tickDelta) {
        bridge.bindPipeline(vulkanContext.getSignalPipeline());

        for (SignalRenderData signal : visibleSignals) {
            float[] modelMatrix = new float[16];
            new Matrix4f()
                    .translate((float) signal.x, (float) signal.y, (float) signal.z)
                    .rotateY(signal.yaw)
                    .get(modelMatrix);

            float[] mvpMatrix = new float[16];
            new Matrix4f(vpMatrix).mul(new Matrix4f(modelMatrix)).get(mvpMatrix);

            // 根据信号状态设置颜色
            float[] color = switch (signal.state) {
                case GREEN -> new float[]{0.2f, 1.0f, 0.2f, 1.0f};
                case YELLOW -> new float[]{1.0f, 1.0f, 0.2f, 1.0f};
                case RED -> new float[]{1.0f, 0.2f, 0.2f, 1.0f};
                default -> new float[]{0.5f, 0.5f, 0.5f, 1.0f};
            };

            bridge.renderMesh("signal_base", mvpMatrix, color);
        }
    }

    public void renderTranslucent(float[] vpMatrix, float tickDelta) {
        // 渲染信号灯的光晕效果
        bridge.bindPipeline(vulkanContext.getTranslucentPipeline());

        for (SignalRenderData signal : visibleSignals) {
            if (signal.state == SignalState.OFF) continue;

            float[] modelMatrix = new float[16];
            new Matrix4f()
                    .translate((float) signal.x, (float) signal.y + 1.5f, (float) signal.z)
                    .get(modelMatrix);

            float[] mvpMatrix = new float[16];
            new Matrix4f(vpMatrix).mul(new Matrix4f(modelMatrix)).get(mvpMatrix);

            float[] glowColor = switch (signal.state) {
                case GREEN -> new float[]{0.2f, 1.0f, 0.2f, 0.3f};
                case YELLOW -> new float[]{1.0f, 1.0f, 0.2f, 0.3f};
                case RED -> new float[]{1.0f, 0.2f, 0.2f, 0.3f};
                default -> new float[]{0, 0, 0, 0};
            };

            // 渲染光晕quad
            bridge.renderMesh("signal_glow", mvpMatrix, glowColor);
        }
    }

    private MeshData createSignalMesh() {
        MeshData mesh = new MeshData();

        // 信号灯柱
        float poleWidth = 0.05f;
        float poleHeight = 2.0f;
        int base = mesh.getVertexCount();

        // 简化为四面柱体
        mesh.addVertex(-poleWidth, 0, -poleWidth, -1, 0, 0, 0, 0, 0.3f, 0.3f, 0.3f, 1);
        mesh.addVertex(-poleWidth, poleHeight, -poleWidth, -1, 0, 0, 0, 1, 0.3f, 0.3f, 0.3f, 1);
        mesh.addVertex(-poleWidth, poleHeight, poleWidth, -1, 0, 0, 1, 1, 0.3f, 0.3f, 0.3f, 1);
        mesh.addVertex(-poleWidth, 0, poleWidth, -1, 0, 0, 1, 0, 0.3f, 0.3f, 0.3f, 1);
        mesh.addQuad(base, base + 1, base + 2, base + 3);

        // 信号灯头
        float headSize = 0.15f;
        base = mesh.getVertexCount();
        mesh.addVertex(-headSize, poleHeight - headSize, headSize, 0, 0, 1, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(headSize, poleHeight - headSize, headSize, 0, 0, 1, 1, 0, 1, 1, 1, 1);
        mesh.addVertex(headSize, poleHeight + headSize, headSize, 0, 0, 1, 1, 1, 1, 1, 1, 1);
        mesh.addVertex(-headSize, poleHeight + headSize, headSize, 0, 0, 1, 0, 1, 1, 1, 1, 1);
        mesh.addQuad(base, base + 1, base + 2, base + 3);

        return mesh;
    }

    public void cleanup() {
        bridge.unregisterMesh("signal_base");
    }

    public enum SignalState { RED, YELLOW, GREEN, OFF }

    public static class SignalRenderData {
        public double x, y, z;
        public float yaw;
        public SignalState state = SignalState.OFF;
    }
}