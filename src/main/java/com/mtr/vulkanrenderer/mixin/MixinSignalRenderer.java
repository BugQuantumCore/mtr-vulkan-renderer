package com.mtr.vulkanrenderer.mixin;

import com.mtr.vulkanrenderer.MTRVulkanRendererClient;
import net.minecraft.client.util.math.MatrixStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * 拦截MTR的信号灯渲染器
 */
@Pseudo
@Mixin(targets = {
        "mtr.client.render.SignalRenderer",
        "mtr.render.SignalRenderer"
}, remap = false)
public class MixinSignalRenderer {

    @Inject(method = "render", at = @At("HEAD"), cancellable = true, remap = false, require = 0)
    private void onRenderSignal(MatrixStack matrices, float tickDelta, CallbackInfo ci) {
        MTRVulkanRendererClient client = MTRVulkanRendererClient.getInstance();
        if (client != null && client.isVulkanAvailable()) {
            ci.cancel();
        }
    }
}