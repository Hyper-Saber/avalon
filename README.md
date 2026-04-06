# ⚔️ Avalon Engine

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)
[![Status](https://img.shields.io/badge/Status-Active_Development-green.svg)]()

**Avalon** 是一款基于 **C++23 Modules** 构建的现代 Vulkan 渲染引擎。架构核心围绕 **Bindless** 资源管理与 **Render Graph** 驱动，旨在将 CPU 侧的调度开销降至最低。

- [核心架构](#核心架构)
  - [1. 世界数据更新 (ECS)](#1-世界数据更新-ecs)
  - [2. 渲染流程 (Render Graph)](#2-渲染流程-render-graph)
  - [3. 渲染数据更新 (Data Binding)](#3-渲染数据更新-data-binding)
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

### 3. 渲染数据更新 (Data Binding)

针对现代 GPU 设计的低开销绑定策略。

| Set | Binding | Resource | Description |
| :--- | :--- | :--- | :--- |
| **Set 0** | 0 | `samplers[]` | **Bindless**: 全局采样器池 |
|  | 1 | `materialDatas` | **SSBO**: 全局材质属性池 (Color, PBR Params, etc.) |
| | 2 | `probes` | **SSBO**: 全局探针池 |
| | 3 | `envCubes[]` | **Bindless**: 全局环境贴图池 |
| | 5 | `texture2DArrays[]` | **Bindless**: 全局材质数组池 |
| | 4 | `volumes[]` | **Bindless**: 全局3D材质池 |
| | 6 | `textures[]` | **Bindless**: 全局材质池 |
| | 7 | `rwTextures[]` | **Bindless**: 全局StorageImage池 |
| | 8 | `rwBuffers` | **SSBO**: 全局通用SSBO池 |
| | 9 | `rwTextureArrays[]` | **Bindless**: 全局storageImageArray池 |
| **Set 1** | 0 | `sceneGlobals` | **UBO**: 相机矩阵、环境光、投影参数 |

---

## 路线图 (Roadmap)

### 🏆 已完成 (Completed)

- [x] **C++23 Modules** 基础脚手架与编译链构建。
- [x] **Render Graph** 自动化同步与资源流转引擎。
- [x] **Bindless** 渲染架构（贴图与采样器全局绑定）。
- [x] **Inverse-Z** 高精度深度缓冲策略。
- [x] **输入系统 (Input System)**: 抽象手柄的三种输入方式,按键,扳机,摇杆。

### 🛠️ 开发中 (In Progress)

- [ ] **PBR 材质系统**: 基于 Cook-Torrance 模型，整合进目前的 Bindless SSBO 布局。

### 🚀 远期目标 (Future)

- [ ] **光线追踪 (Ray Tracing)**: 接入 `VK_KHR_ray_tracing` 扩展，实现硬核阴影与反射。
- [ ] **Async Compute**: 利用 Render Graph 实现自动异步计算分发。
- [ ] **Meshlet Pipeline**: 探索基于任务着色器的现代渲染管线。
- [ ] **Hot reload**: 在上层应用运行时提供编辑器模式,支持gameplay等上层模块的热重载。

---
