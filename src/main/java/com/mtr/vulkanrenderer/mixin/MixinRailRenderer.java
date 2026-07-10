package com.mtr.vulkanrenderer.mixin;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;
import com.mtr.vulkanrenderer.MTRVulkanRendererClient;
import net.minecraft.client.util.math.MatrixStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * 拦截MTR的轨道渲染器
 * 目标类: mtr.client.render.RailRenderer 或 mtr.client.render.RenderRail
 */
@Pseudo
@Mixin(targets = {
        "mtr.client.render.RailRenderer",
        "mtr.client.render.RenderRail",
        "mtr.render.RenderRail"
}, remap = false)
public class MixinRailRenderer {

    @Inject(method = "render", at = @At("HEAD"), cancellable = true, remap = false, require = 0)
    private void onRenderRail(MatrixStack matrices, float tickDelta, CallbackInfo ci) {
        MTRVulkanRendererClient client = MTRVulkanRendererClient.getInstance();
        if (client != null && client.isVulkanAvailable()) {
            ci.cancel();
        }
    }

    /**
     * 拦截轨道区块的渲染
     * MTR可能按区块组织轨道渲染
     */
    @Inject(method = "renderChunk", at = @At("HEAD"), cancellable = true, remap = false, require = 0)
    private void onRenderChunk(CallbackInfo ci) {
        MTRVulkanRendererClient client = MTRVulkanRendererClient.getInstance();
        if (client != null && client.isVulkanAvailable()) {
            ci.cancel();
        }
    }
}