# Avalon Engine 架构图 (Architecture Manifest)

**版本:** 0.1
**状态:** 正在开发 (Shared DLL 模式)

1. 模块说明 (Module Definitions)
引擎由四个物理层级组成，通过动态链接和接口注入实现解耦。

A. 应用集成层 (Application Layer)
avalon.engine (Shared DLL): 引擎的“大脑”与门面。负责通过 PluginLoader 初始化硬件插件，并协调渲染、输入、场景等子系统的运行。

PluginLoader (Internal Utility): 封装 dlopen/dlsym (Linux) 或 LoadLibrary/GetProcAddress (Windows)，负责在运行时发现并注入 .so 插件。

B. 功能子系统层 (Functional Subsystems)
renderer: 跨平台的渲染逻辑（如延迟渲染管线、阴影计算）。它只对 avalon.rhi 的接口编程。

scene & ecs: 负责数据组织。ecs 提供高效的内存布局，scene 提供层级化的对象管理。

resource & vfs: 处理资源生命周期。vfs 隐藏了物理路径和压缩包的差异。

C. 硬件抽象层 (HAL - Interface & Plugins)
avalon.rhi / avalon.window: 纯虚接口层。定义了“如何绘图”和“如何开窗”的标准，不含具体实现。

avalon_rhi_vulkan.so (Dynamic Plugin): 包含 Vulkan SDK 调用的具体实现。

avalon_window_glfw.so (Dynamic Plugin): 包含 GLFW 调用的具体实现。

D. 全局基石层 (Foundation Layer)
avalon.core (Shared DLL): 全引擎唯一的物理核心。提供 spdlog 日志、GLM 数学库、自定义内存分配器以及多线程任务图。

1. 设计原则 (Design Principles)
I. 唯一核心原则 (Single Source of Truth)
为了彻底消除菱形依赖，avalon.core 必须作为 Shared DLL 存在。所有其他模块（Renderer, RHI 插件等）在运行时必须链接到同一个内存地址的 Core，确保全局状态（如日志级别、内存计数）的唯一性。

II. 后端不透明性 (Backend Opacity)
用户项目（Game）和功能层（Renderer）永远不应该包含 <vulkan/vulkan.h>。所有的硬件操作必须通过虚接口进行。这保证了你可以在不重新编译引擎核心的情况下，通过替换 .so 文件来升级驱动后端。

III. 插件化生命周期 (Plugin Lifecycle)
插件的创建（Create）和销毁（Destroy）必须在 DLL 内部配对。

原则： “谁申请，谁释放”。

实现： 接口使用 virtual 析构函数，或者通过共享库导出的 DestroyInstance 函数来安全回收内存，防止跨 DLL 堆栈破坏。

1. 具体架构图 (Architecture Tree)

```mermaid
%%{init: {'theme': 'dark', 'flowchart': {'htmlLabels': true, 'nodeSpacing': 50, 'rankSpacing': 80}}}%%
graph LR
    %% ============================================================
    %% 1. 全局默认样式提取 (无单独字体设置，全靠 default 驱动)
    %% ============================================================
    classDef default font-size:16px,font-weight:bold,color:#FFFFFF,stroke:#000,stroke-width:2px;

    %% ============================================================
    %% 2. 覆盖各层级背景色e
    %% ============================================================
    classDef user fill:#0D47A1;
    classDef framework fill:#1B5E20;
    classDef internal fill:#E65100;
    classDef interface fill:#37474F;
    classDef plugin fill:#4A148C,stroke-dasharray: 8 4,stroke-width:3px;
    classDef core fill:#B71C1C;

    %% ============================================================
    %% 3. 架构拓扑 (LR 布局)
    %% ============================================================

    subgraph Layer_A [Application Layer]
        Game["GAME_APP"] --> Engine["AVALON_ENGINE"]
        Engine --- PLoader["PLUGIN_LOADER"]
    end

    subgraph Layer_B [Functional Subsystems]
        Engine --> Renderer["RENDER_LOGIC"]
        Engine --> Resource["RESOURCE_VFS"]
        Engine --> Scene["SCENE_ECS"]
    end

    subgraph Layer_C_IF [HAL Interfaces]
        Renderer --> RHI_I["AVALON_RHI_API"]
        Engine --> Win_I["AVALON_WINDOW_API"]
    end

    subgraph Layer_C_PL [Dynamic Plugins]
        VK_P["VULKAN_BACKEND"]
        DX_P["DX12_BACKEND"]
        GLFW_P["GLFW_WINDOW"]
        SComp["SHADER_COMPILER"]
    end

    %% 注入逻辑 (PluginLoader -> Plugins)
    PLoader -.->|"INJECT"| VK_P
    PLoader -.->|"INJECT"| DX_P
    PLoader -.->|"INJECT"| GLFW_P
    PLoader -.->|"INJECT"| SComp

    %% 接口挂载 (Plugins -> Interfaces)
    VK_P -.-> RHI_I
    DX_P -.-> RHI_I
    GLFW_P -.-> Win_I
    
    %% 逻辑关联
    Renderer --- SComp

    subgraph Layer_D [Foundation Layer]
        Core["AVALON_CORE_SSOT"]
    end

    %% 全局连接到核心 (消除所有模块的依赖歧义)
    Engine & Renderer & Resource & Scene & VK_P & DX_P & GLFW_P & SComp & PLoader --> Core

    %% ============================================================
    %% 4. 样式映射
    %% ============================================================
    class Game user;
    class Engine,Renderer,Resource,Scene framework;
    class PLoader internal;
    class RHI_I,Win_I interface;
    class VK_P,DX_P,GLFW_P,SComp plugin;
    class Core core;
