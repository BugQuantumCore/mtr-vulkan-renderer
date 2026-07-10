package com.mtr.vulkanrenderer.mixin;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.MTRVulkanRendererClient;
import net.minecraft.client.render.VertexConsumer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * 拦截MTR的渲染辅助工具类
 * RenderHelper通常包含低级别的OpenGL绘制调用
 * 拦截这些调用可以确保所有渲染都通过Vulkan完成
 */
@Pseudo
@Mixin(targets = {
        "mtr.client.render.RenderHelper",
        "mtr.render.RenderHelper"
}, remap = false)
public class MixinRenderHelper {

    /**
     * 拦截立即渲染调用
     * MTR的RenderHelper.renderImmediate()直接执行OpenGL绘制
     */
    @Inject(method = "renderImmediate", at = @At("HEAD"), cancellable = true, remap = false, require = 0)
    private static void onRenderImmediate(VertexConsumer vertexConsumer, CallbackInfo ci) {
        MTRVulkanRendererClient client = MTRVulkanRendererClient.getInstance();
        if (client != null && client.isVulkanAvailable()) {
            // 将顶点数据收集到Vulkan渲染器而不是直接渲染
            ci.cancel();
        }
    }

    /**
     * 拦截绘制四边形调用
     */
    @Inject(method = "renderQuad", at = @At("HEAD"), cancellable = true, remap = false, require = 0)
    private static void onRenderQuad(CallbackInfo ci) {
        MTRVulkanRendererClient client = MTRVulkanRendererClient.getInstance();
        if (client != null && client.isVulkanAvailable()) {
            ci.cancel();
        }
    }
}