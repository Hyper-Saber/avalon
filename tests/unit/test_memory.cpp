#include <iostream>
#include <vector>

import avalon.core;
import test.utils;

using namespace avalon;
using namespace test_config;

// 集成测试：模拟一个完整的资源加载和释放流程
void test_memory_lifecycle_contract() {
  // 记录初始基准线（不需要假设初始为 0，因为可能有全局静态变量占用）
  const size_t baseUsage = mem::GetTotalUsage();
  const size_t baseCount = mem::GetAllocCount();

  {
    // 模拟资源加载
    const size_t dataSize = 1024;
    Array<std::byte> buffer;
    buffer.resize(dataSize);

    // 契约 1：分配必须被追踪
    expect(mem::GetTotalUsage() >= baseUsage + dataSize,
           "Usage should increase after allocation");
    expect(mem::GetAllocCount() > baseCount, "Alloc count should increase");

    // 模拟交给引擎处理
    auto blob = CreateBlob(std::move(buffer));

    // 契约 2：所有权转移（这是黑盒必须保证的语义）
    expect(buffer.empty(),
           "Source container must be empty after handing over to Blob");
    expect(blob->GetSize() == dataSize, "Blob must report the same data size");
  }

  // 契约 3：生命周期结束，必须完美归还内存
  // 无论内部是 ArrayBlob 还是别的，只要销毁了，账目必须对齐
  expect(mem::GetTotalUsage() == baseUsage,
         "Memory leak detected: usage did not return to baseline");
  expect(mem::GetAllocCount() == baseCount,
         "Memory leak detected: alloc count did not return to baseline");
}

// 统计测试：只关心业务指标
void test_memory_metric_contract() {
  const size_t initialTransfer = mem::GetTotalTransferUsage();

  std::vector<uint8_t> src(100, 1);
  std::vector<uint8_t> dst(100, 0);

  mem::InternalMemcpy(dst.data(), src.data(), 100);

  // 契约：搬运量必须增加，不关心底层是怎么实现的
  expect(mem::GetTotalTransferUsage() == initialTransfer + 100,
         "Transfer metric mismatch");
}

int main() {
  std::cout << "--- [Avalon Memory Contract Test] ---" << std::endl;

  test_memory_lifecycle_contract();
  test_memory_metric_contract();

  std::cout << "--- [All Contracts Verified] ---" << std::endl;
  return 0;
}
