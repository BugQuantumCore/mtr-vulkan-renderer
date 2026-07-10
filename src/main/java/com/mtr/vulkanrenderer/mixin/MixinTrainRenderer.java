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
 * 拦截MTR的列车渲染器。
 *
 * 当MTR尝试通过OpenGL渲染列车时，我们拦截该调用，
 * 转而使用Vulkan进行渲染。
 *
 * 目标类: mtr.client.render.TrainRenderer (MTR 4.0.6)
 * 使用@Pseudo因为目标类在运行时才存在
 */
@Pseudo
@Mixin(targets = "mtr.client.render.TrainRenderer", remap = false)
public class MixinTrainRenderer {

    /**
     * 拦截列车渲染方法
     * MTR的TrainRenderer.render(MatrixStack, float)被调用时，
     * 取消原始OpenGL渲染，改为收集渲染数据
     */
    @Inject(method = "render", at = @At("HEAD"), cancellable = true, remap = false)
    private void onRender(MatrixStack matrices, float tickDelta, CallbackInfo ci) {
        MTRVulkanRendererClient client = MTRVulkanRendererClient.getInstance();
        if (client != null && client.isVulkanAvailable()) {
            // Vulkan渲染已接管，取消MTR的原始OpenGL渲染
            // 实际的Vulkan渲染在RenderDispatcher中通过WorldRenderEvents完成
            ci.cancel();

            if (MTRVulkanRendererClient.getInstance().getConfig().logRenderCalls) {
                MTRVulkanRenderer.LOGGER.debug("[MTR-Vulkan] Intercepted train render call, using Vulkan");
            }
        }
        // 如果Vulkan不可用，让MTR继续使用OpenGL渲染
    }

    /**
     * 拦截列车渲染的尾部调用
     * 如果我们不取消HEAD，可以在TAIL收集数据
     */
    @Inject(method = "render", at = @At("TAIL"), remap = false)
    private void afterRender(MatrixStack matrices, float tickDelta, CallbackInfo ci) {
        // 可用于收集渲染后的统计信息
    }
}