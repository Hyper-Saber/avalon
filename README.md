# ⚔️ Avalon Engine

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
[![Status](https://img.shields.io/badge/Status-Active_Development-green.svg)]()

**Avalon** 是一款基于 **C++23 Modules** 构建的现代 Vulkan 渲染引擎。架构核心围绕 **Bindless** 资源管理与 **Render Graph** 驱动，旨在将 CPU 侧的调度开销降至最低。

- [核心架构](#核心架构)
  - [1. 世界数据更新 (ECS)](#1-世界数据更新-ecs)
  - [2. 渲染流程 (Render Graph)](#2-渲染流程-render-graph)
  - [3. 渲染管线资源绑定表 (Data Binding)](#3-渲染管线资源绑定表-descriptor-sets)
- [路线图 (Roadmap)](#路线图-roadmap)

---

## 核心架构

### 1. 世界数据更新 (ECS)

系统逻辑同步与高性能数据管理层。

- **架构**: 基于 **Entity Component System (ECS)**。
- **优势**: 保证了缓存友好的内存布局，支持大规模实体的并行更新。

### 2. 渲染流程 (Render Graph)

- **同步优化**: 自动分析资源读写依赖，精确注入 **Pipeline Barriers** 与 **Image Layout Transitions**。
- **资源管理**: 自动化处理资源生命周期，支持显存复用（Aliasing）。

### 3. 渲染管线资源绑定表 (Descriptor Sets)

| Set | Binding | HLSL 变量名 | 类型 (HLSL) | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| **Set 0** | 0 | `uSamplers[]` | `SamplerState` | **Bindless**: 全局采样器池 |
| | 1 | `uMaterials` | `StructuredBuffer<MaterialData>` | **SSBO**: 全局材质属性池 (PBR 参数、颜色等) |
| | 2 | `uStaticSSBO` | `ByteAddressBuffer` | **SSBO**: 静态几何/场景/通用数据 |
| | 3 | `uDynamicSSBO` | `ByteAddressBuffer` | **SSBO**: 动态/每帧更新数据 (Model 矩阵等) |
| | 4 | `uPosUVSSBO` | `ByteAddressBuffer` | **SSBO**: 顶点位置 (Position) 与 UV 数据 |
| | 5 | `uAttributesSSBO` | `ByteAddressBuffer` | **SSBO**: 顶点法线、切线、顶点色等属性 |
| | 6 | `uIndicesSSBO` | `ByteAddressBuffer` | **SSBO**: 全局索引缓冲区 |
| | 7 | `uCommandSSBO` | `StructuredBuffer<DrawCommand>` | **SSBO**: 绘制命令数据 (DrawCommand / Indirect Args) |
| | 8 | `uEnvCubes[]` | `TextureCube` | **Bindless**: 全局环境贴图池 |
| | 9 | `uTextureArrays[]` | `Texture2DArray` | **Bindless**: 全局纹理数组池 |
| | 10 | `uVolumes[]` | `Texture3D` | **Bindless**: 全局 3D 纹理/体积数据 |
| | 11 | `uTextures[]` | `Texture2D` | **Bindless**: 全局 2D 纹理池 |
| | 12 | `uRWTextures[]` | `RWTexture2D` | **Bindless**: 可读写 Storage Image |
| | 13 | `uRWTextureArrays[]`| `RWTexture2DArray` | **Bindless**: 可读写 Storage Image Array |
| **Set 1** | 0 | `sceneGlobals` | `ConstantBuffer<SceneGlobals>` | **UBO**: 场景全局参数 (View/Proj 矩阵、时间、灯光) |

---

## 路线图 (Roadmap)

### 🏆 已完成 (Completed)

- [x] **C++23 Modules** 基础脚手架与编译链构建。
- [x] **Render Graph** 自动化同步与资源流转引擎。
- [x] **Bindless** 渲染架构（贴图与采样器全局绑定）。
- [x] **Inverse-Z** 高精度深度缓冲策略。
- [x] **输入系统 (Input System)**: 抽象手柄的三种输入方式,按键,扳机,摇杆。
- [x] **Indirect draw**: 支持间接渲染。

### 🛠️ 开发中 (In Progress)

- [ ] **PBR 材质系统**: 基于BRDF, D项使用GGX。

### 🚀 远期目标 (Future)

- [ ] **光线追踪 (Ray Tracing)**: 接入 `VK_KHR_ray_tracing` 扩展，实现硬核阴影与反射。
- [ ] **Async Compute**: 利用 Render Graph 实现自动异步计算分发。
- [ ] **Meshlet Pipeline**: 探索基于任务着色器的现代渲染管线。
- [ ] **Hot reload**: 在上层应用运行时提供编辑器模式,支持gameplay等上层模块的热重载。

---
