# MTR Vulkan Renderer

一个将 **Minecraft Transit Railway** 的 OpenGL 渲染器替换为 **Vulkan API** 的 Fabric 模组，使用 C++ 原生代码实现高性能渲染。

## 🎯 项目目标

- 将 MTR 的列车、轨道、信号、车站等所有渲染操作从 OpenGL 迁移到 Vulkan
- 利用 Vulkan 的低开销和高并行性提升渲染性能
- 支持实例化渲染、GPU 驱动渲染等现代图形技术
- 与 [VulkanMod](https://github.com/xCollateral/VulkanMod) 共享 Vulkan 上下文

## ✨ 特性

- 🚄 **列车渲染**：支持实例化渲染，可同时渲染数百列列车
- 🛤️ **轨道渲染**：按区块缓存网格，支持贝塞尔曲线轨道
- 🚦 **信号渲染**：动态信号灯颜色和光晕效果
- 🏢 **车站渲染**：屏蔽门、电梯、扶梯等车站设施
- 🎨 **自定义着色器**：优化的 GLSL 着色器，支持 Phong 光照
- 📊 **性能统计**：实时显示绘制调用、三角形数量、帧时间
- 🔧 **可配置**：丰富的配置选项，支持调试模式

## 📋 系统要求

### 必需

- **Minecraft Java Edition 1.20.1**
- **Fabric Loader 0.14.14+**
- **Fabric API**
- **Minecraft Transit Railway**
- **VulkanMod**
- **支持 Vulkan 1.2 的显卡**

### 推荐

- NVIDIA RTX 2000+ / AMD RX 6000+ 显卡
- 8GB+ 显存
- 16GB+ 系统内存

## 🚀 安装

### 用户安装

1. 安装 Minecraft 1.20.1 和 [Fabric Loader](https://www.fabricmc.net/use/installer/)
2. 安装以下必需模组：
   - [Fabric API](https://modrinth.com/mod/fabric-api)
   - [Minecraft Transit Railway](https://modrinth.com/mod/minecraft-transit-railway)
   - [VulkanMod](https://modrinth.com/mod/vulkanmod)
3. 下载 MTR Vulkan Renderer 模组 (https://github.com/BugQuantumCore/mtr-vulkan-renderer/releases)
4. 将下载的 JAR 文件放入 `.minecraft/mods/` 文件夹
5. 启动游戏

### 开发环境设置

```bash
# 克隆仓库
git clone https://github.com/yourusername/mtr-vulkan-renderer.git
cd mtr-vulkan-renderer

# 安装 Vulkan SDK
# Windows: 从 https://vulkan.lunarg.com/sdk/home 下载安装
# Linux (Ubuntu/Debian):
sudo apt install vulkan-sdk
# macOS:
brew install vulkan-sdk

# 设置环境变量（如果需要）
export VULKAN_SDK=/path/to/vulkan-sdk

# 构建项目
./gradlew build

# 运行游戏（开发模式）
./gradlew runClient