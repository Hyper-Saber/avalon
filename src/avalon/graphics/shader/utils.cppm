module;
export module avalon.shader:utils;

import avalon.core;
import avalon.rhi;
import :serialization;

export namespace avalon::graphics {
auto ToView(rhi::EShaderStage stage) {
  switch (stage) {
  case rhi::EShaderStage::Vertex:
    return kDefaultVsEntryPointName;
  case rhi::EShaderStage::Fragment:
    return kDefaultFsEntryPointName;
  case rhi::EShaderStage::Compute:
    return kDefaultCsEntryPointName;
  default:
    return StringView::kEmptyView;
    ;
  }
}
} // namespace avalon::graphics
