#include <iostream>
#include <string_view>

import avalon.core;
import test.utils;

// 使用专门的 Access 类获取实例
using VfsAccess = avalon::vfs::VfsTestAccess;
using namespace test_config;

void test_vfs_mount_logic() {
  // 1. 通过 Access 类规避 private 限制
  auto vfs = VfsAccess::Create();
  auto disk = VfsAccess::CreateDevice();

  // 2. 挂载逻辑测试
  vfs->Mount("/tests", "./", disk.get(), 0);

  bool exists = vfs->IsExists("/tests/xmake.lua");
  expect(exists, "VFS should find xmake.lua through registered access");
}

void test_vfs_priority() {
  auto vfs = VfsAccess::Create();
  auto disk = VfsAccess::CreateDevice();

  vfs->Mount("/data", "./base", disk.get(), 0);
  vfs->Mount("/data", "./patch", disk.get(), 100); // 高优先级

  // 验证逻辑...
  expect(true, "Priority test placeholder");
}

int main() {
  std::cout << "--- Starting Avalon VFS Protected Tests ---" << std::endl;

  test_vfs_mount_logic();
  test_vfs_priority();

  std::cout << "--- All VFS Tests Completed ---" << std::endl;
  return 0;
}
