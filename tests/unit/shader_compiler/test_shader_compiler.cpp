#include <format>
#include <iostream>
#include <string_view>

import avalon.core;
import avalon.shader_compiler;
import test.utils;

// 依然通过 VfsAccess 辅助类进行测试环境配置
using VfsAccess = avalon::vfs::VfsTestAccess;
using namespace test_config;

void test_shader_compilation_from_vfs() {
  // 1. 初始化环境 (模拟 Engine 启动流程)
  auto vfs = VfsAccess::Create();
  auto disk = VfsAccess::CreateDevice();

  // 挂载资源目录：物理 ./assets -> 虚拟 /shaders
  // 注意：这里路径要根据你 xmake 运行的 CWD 调整
  vfs->Mount("/shaders", "./tests/unit/shader_compiler", disk.get(), 0);

  // 将 vfs 注入全局 Context，这样插件内部才能通过 avalon::GetContext().pVfs
  // 拿到它
  avalon::GetContext().pVfs = vfs.get();

  // 2. 加载 ShaderCompiler 插件
  // 假设插件名字是 libavalon.shader_compiler.so
  const std::string pluginPath =
      std::format("libavalon.shader_compiler{}", avalon::kPluginExtension);
  auto loadResult =
      avalon::LoadPlugin<avalon::shader_compiler::IShaderCompiler>(pluginPath);

  expect(loadResult.has_value(),
         "Shader compiler plugin should load successfully");

  if (loadResult) {
    auto &compiler = loadResult.value();

    // 3. 执行编译测试

    auto result = compiler->CompileFromFile(
        "/shaders/test.hlsl", "Main",
        avalon::shader_compiler::EShaderStage::Vertex);

    expect(result.has_value(),
           "Shader should compile successfully from VFS path");

    if (!result) {
      // 如果失败，打印错误信息（假设你的 Error 类型支持格式化）
      std::cerr << "Compile Error: " << (int)result.error() << std::endl;
    } else {
      auto compileResult = result.value();
      if (compileResult)
        std::cout << "Shader compiled successfully! Blob size: "
                  << result.value().bytecode.size() << " bytes." << std::endl;
      else {
        std::cout << compileResult.errorMessage << std::endl;
      }
    }
  }
}

int main() {
  std::cout << "--- Starting Shader Compiler Integration Tests ---"
            << std::endl;

  test_shader_compilation_from_vfs();

  std::cout << "--- Shader Tests Completed ---" << std::endl;
  return 0;
}
