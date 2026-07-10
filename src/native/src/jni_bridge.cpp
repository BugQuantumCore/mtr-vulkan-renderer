#include <jni.h>
#include "mtr_vulkan_renderer.h"
#include <memory>
#include <string>
#include <cstring>
#include <chrono>

// 全局渲染器实例
static std::unique_ptr<MTRVulkanRenderer> g_renderer;

// ==================== 辅助函数 ====================

static std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// ==================== JNI 方法实现 ====================

extern "C" {

// ---------- 初始化/清理 ----------

JNIEXPORT jboolean JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_initializeVulkan(
        JNIEnv* env, jobject thiz) {
    g_renderer = std::make_unique<MTRVulkanRenderer>();
    return g_renderer->initialize() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_cleanup(
        JNIEnv* env, jobject thiz) {
    if (g_renderer) {
        g_renderer->cleanup();
        g_renderer.reset();
    }
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_onWindowResize(
        JNIEnv* env, jobject thiz, jint width, jint height) {
    if (g_renderer) {
        g_renderer->onWindowResize(width, height);
    }
}

// ---------- 帧管理 ----------

JNIEXPORT jint JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_beginFrame(
        JNIEnv* env, jobject thiz) {
    if (!g_renderer) return -1;
    return g_renderer->beginFrame();
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_endFrame(
        JNIEnv* env, jobject thiz) {
    if (g_renderer) g_renderer->endFrame();
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_waitForFrame(
        JNIEnv* env, jobject thiz) {
    if (g_renderer) g_renderer->waitForFrame();
}

// ---------- 管线管理 ----------

JNIEXPORT jlong JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_createPipeline(
        JNIEnv* env, jobject thiz,
        jstring vertexShaderPath, jstring fragmentShaderPath,
        jstring vertexFormat, jint topology,
        jboolean blendEnabled, jboolean depthTestEnabled) {
    if (!g_renderer) return 0;

    auto vert = jstringToString(env, vertexShaderPath);
    auto frag = jstringToString(env, fragmentShaderPath);
    auto format = jstringToString(env, vertexFormat);

    return g_renderer->createPipeline(
            vert.c_str(), frag.c_str(), format.c_str(),
            topology, blendEnabled, depthTestEnabled);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_destroyPipeline(
        JNIEnv* env, jobject thiz, jlong handle) {
    if (g_renderer) g_renderer->destroyPipeline(handle);
}

// ---------- 缓冲区管理 ----------

JNIEXPORT jlong JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_createVertexBuffer(
        JNIEnv* env, jobject thiz, jint sizeBytes) {
    if (!g_renderer) return 0;
    return g_renderer->createVertexBuffer(sizeBytes);
}

JNIEXPORT jlong JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_createIndexBuffer(
        JNIEnv* env, jobject thiz, jint sizeBytes) {
    if (!g_renderer) return 0;
    return g_renderer->createIndexBuffer(sizeBytes);
}

JNIEXPORT jlong JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_createUniformBuffer(
        JNIEnv* env, jobject thiz, jint sizeBytes) {
    if (!g_renderer) return 0;
    return g_renderer->createUniformBuffer(sizeBytes);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_uploadVertexData(
        JNIEnv* env, jobject thiz, jlong bufferHandle,
        jfloatArray data, jint offset) {
    if (!g_renderer || !data) return;

    jsize length = env->GetArrayLength(data);
    jfloat* elements = env->GetFloatArrayElements(data, nullptr);

    g_renderer->uploadVertexData(bufferHandle, elements, length, offset);

    env->ReleaseFloatArrayElements(data, elements, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_uploadIndexData(
        JNIEnv* env, jobject thiz, jlong bufferHandle,
        jintArray data, jint offset) {
    if (!g_renderer || !data) return;

    jsize length = env->GetArrayLength(data);
    jint* elements = env->GetIntArrayElements(data, nullptr);

    g_renderer->uploadIndexData(bufferHandle, elements, length, offset);

    env->ReleaseIntArrayElements(data, elements, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_uploadUniformData(
        JNIEnv* env, jobject thiz, jlong bufferHandle,
        jbyteArray data, jint offset) {
    if (!g_renderer || !data) return;

    jsize length = env->GetArrayLength(data);
    jbyte* elements = env->GetByteArrayElements(data, nullptr);

    g_renderer->uploadUniformData(bufferHandle,
            reinterpret_cast<const uint8_t*>(elements), length, offset);

    env->ReleaseByteArrayElements(data, elements, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_destroyBuffer(
        JNIEnv* env, jobject thiz, jlong handle) {
    if (g_renderer) g_renderer->destroyBuffer(handle);
}

// ---------- 纹理管理 ----------

JNIEXPORT jlong JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_createTexture(
        JNIEnv* env, jobject thiz,
        jbyteArray pixelData, jint width, jint height, jint mipLevels) {
    if (!g_renderer || !pixelData) return 0;

    jsize length = env->GetArrayLength(pixelData);
    jbyte* pixels = env->GetByteArrayElements(pixelData, nullptr);

    jlong result = g_renderer->createTexture(
            reinterpret_cast<const uint8_t*>(pixels), width, height, mipLevels);

    env->ReleaseByteArrayElements(pixelData, pixels, JNI_ABORT);
    return result;
}

JNIEXPORT jlong JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_createTextureFromGL(
        JNIEnv* env, jobject thiz,
        jint glTextureId, jint width, jint height) {
    if (!g_renderer) return 0;
    return g_renderer->createTextureFromGL(glTextureId, width, height);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_destroyTexture(
        JNIEnv* env, jobject thiz, jlong handle) {
    if (g_renderer) g_renderer->destroyTexture(handle);
}

// ---------- 渲染命令 ----------

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_bindPipeline(
        JNIEnv* env, jobject thiz, jlong handle) {
    if (g_renderer) g_renderer->bindPipeline(handle);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_bindVertexBuffer(
        JNIEnv* env, jobject thiz, jlong handle) {
    if (g_renderer) g_renderer->bindVertexBuffer(handle);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_bindIndexBuffer(
        JNIEnv* env, jobject thiz, jlong handle) {
    if (g_renderer) g_renderer->bindIndexBuffer(handle);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_bindTexture(
        JNIEnv* env, jobject thiz, jlong handle, jint slot) {
    if (g_renderer) g_renderer->bindTexture(handle, slot);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_setUniforms(
        JNIEnv* env, jobject thiz,
        jfloatArray mvpMatrix, jfloatArray modelMatrix,
        jfloatArray lightDirection, jfloat ambientLight) {
    if (!g_renderer) return;

    jfloat* mvp = mvpMatrix ? env->GetFloatArrayElements(mvpMatrix, nullptr) : nullptr;
    jfloat* model = modelMatrix ? env->GetFloatArrayElements(modelMatrix, nullptr) : nullptr;
    jfloat* light = lightDirection ? env->GetFloatArrayElements(lightDirection, nullptr) : nullptr;

    g_renderer->setUniforms(mvp, model, light, ambientLight);

    if (mvp) env->ReleaseFloatArrayElements(mvpMatrix, mvp, JNI_ABORT);
    if (model) env->ReleaseFloatArrayElements(modelMatrix, model, JNI_ABORT);
    if (light) env->ReleaseFloatArrayElements(lightDirection, light, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_drawIndexed(
        JNIEnv* env, jobject thiz, jint indexCount, jint instanceCount) {
    if (g_renderer) g_renderer->drawIndexed(indexCount, instanceCount);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_drawArrays(
        JNIEnv* env, jobject thiz, jint vertexCount, jint instanceCount) {
    if (g_renderer) g_renderer->drawArrays(vertexCount, instanceCount);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_drawIndirect(
        JNIEnv* env, jobject thiz, jintArray commands) {
    if (!g_renderer || !commands) return;

    jsize length = env->GetArrayLength(commands);
    jint* elements = env->GetIntArrayElements(commands, nullptr);

    g_renderer->drawIndirect(elements, length / 5);

    env->ReleaseIntArrayElements(commands, elements, JNI_ABORT);
}

// ---------- 网格管理 ----------

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_registerMesh(
        JNIEnv* env, jobject thiz,
        jstring meshId, jfloatArray vertices, jintArray indices, jlong textureHandle) {
    if (!g_renderer || !meshId || !vertices || !indices) return;

    auto id = jstringToString(env, meshId);

    jsize vLen = env->GetArrayLength(vertices);
    jsize iLen = env->GetArrayLength(indices);
    jfloat* vData = env->GetFloatArrayElements(vertices, nullptr);
    jint* iData = env->GetIntArrayElements(indices, nullptr);

    int vertexCount = vLen / 12; // 12 floats per vertex
    g_renderer->registerMesh(id.c_str(), vData, vertexCount, iData, iLen, textureHandle);

    env->ReleaseFloatArrayElements(vertices, vData, JNI_ABORT);
    env->ReleaseIntArrayElements(indices, iData, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_renderMesh(
        JNIEnv* env, jobject thiz,
        jstring meshId, jfloatArray mvpMatrix, jfloatArray color) {
    if (!g_renderer || !meshId || !mvpMatrix || !color) return;

    auto id = jstringToString(env, meshId);
    jfloat* mvp = env->GetFloatArrayElements(mvpMatrix, nullptr);
    jfloat* col = env->GetFloatArrayElements(color, nullptr);

    g_renderer->renderMesh(id.c_str(), mvp, col);

    env->ReleaseFloatArrayElements(mvpMatrix, mvp, JNI_ABORT);
    env->ReleaseFloatArrayElements(color, col, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_unregisterMesh(
        JNIEnv* env, jobject thiz, jstring meshId) {
    if (!g_renderer || !meshId) return;
    auto id = jstringToString(env, meshId);
    g_renderer->unregisterMesh(id.c_str());
}

// ---------- 调试 ----------

JNIEXPORT jstring JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_getGPUInfo(
        JNIEnv* env, jobject thiz) {
    if (!g_renderer) return env->NewStringUTF("Not initialized");
    auto info = g_renderer->getGPUInfo();
    return env->NewStringUTF(info.c_str());
}

JNIEXPORT jfloatArray JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_getRenderStats(
        JNIEnv* env, jobject thiz) {
    float stats[4] = {0, 0, 0, 0};
    if (g_renderer) {
        g_renderer->getRenderStats(stats);
    }
    jfloatArray result = env->NewFloatArray(4);
    env->SetFloatArrayRegion(result, 0, 4, stats);
    return result;
}

JNIEXPORT void JNICALL
Java_com_mtr_vulkanrenderer_bridge_VulkanBridge_debugMarker(
        JNIEnv* env, jobject thiz,
        jstring name, jfloat r, jfloat g, jfloat b) {
    if (!g_renderer || !name) return;
    auto n = jstringToString(env, name);
    g_renderer->debugMarker(n.c_str(), r, g, b);
}

} // extern "C"