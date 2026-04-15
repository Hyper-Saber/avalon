module;
#include <cstdint>
#include <optional>
export module avalon.rhi:state_tracker;

import avalon.core;
import :types;
import :command_buffer;
import :utils;

export namespace avalon::rhi {

class AVALON_RHI_API StateTracker final
    : public mem::AutoDestroyable<StateTracker> {
public:
  auto RequestSync(ICommandBuffer &cmd, TextureHandle texture,
                   EResourceUsage nextUsage, uint32_t layerCount,
                   uint32_t levelCount, EShaderStage nextStage)
      -> std::optional<ImageBarrier> {

    auto state = m_textureStates.Get(texture);

    if (!state) {
      TextureState initialState{
          .currentUsage = EResourceUsage::None,
          .lastWriteStage = EPipelineStage::None,
          .currentLayout = EResourceLayout::Undefined,
      };

      auto barrier = CreateBarrier(texture, initialState, nextUsage, layerCount,
                                   levelCount, nextStage);

      m_textureStates.Insert(texture,
                             {.currentUsage = nextUsage,
                              .lastWriteStage = IsWriteUsage(nextUsage)
                                                    ? barrier.dstStage
                                                    : EPipelineStage::None,
                              .currentLayout = barrier.newLayout});
      return barrier;
    }

    auto nextLayout = MapUsageToLayout(nextUsage);

    if (state->currentLayout == nextLayout &&
        !IsWriteUsage(state->currentUsage) && !IsWriteUsage(nextUsage)) {
      return std::nullopt;
    }

    auto barrier = CreateBarrier(texture, *state, nextUsage, layerCount,
                                 levelCount, nextStage);

    state->currentUsage = nextUsage;
    state->currentShaderStage = nextStage;
    if (IsWriteUsage(nextUsage)) {
      state->lastWriteStage = barrier.dstStage;
    } else if (barrier.newLayout != state->currentLayout) {
      state->lastWriteStage = EPipelineStage::None;
    }
    state->currentLayout = barrier.newLayout;

    return barrier;
  }

  auto RequestSync(ICommandBuffer &cmd, BufferHandle handle,
                   EResourceUsage nextUsage, uint32_t offset, uint32_t size,
                   EShaderStage stage) -> std::optional<BufferBarrier> {

    auto state = m_bufferStates.Get(handle);

    if (!state) {
      BufferState initialState{
          .currentUsage = EResourceUsage::None,
          .lastWriteStage = EPipelineStage::None,
      };

      auto barrier =
          CreateBarrier(handle, initialState, nextUsage, offset, size, stage);

      m_bufferStates.Insert(handle,
                            {.currentUsage = nextUsage,
                             .lastWriteStage = IsWriteUsage(nextUsage)
                                                   ? barrier.dstStage
                                                   : EPipelineStage::None});
      return barrier;
    }

    if (state->currentUsage == nextUsage && !IsWriteUsage(nextUsage)) {
      return std::nullopt;
    }

    auto barrier =
        CreateBarrier(handle, *state, nextUsage, offset, size, stage);

    state->currentUsage = nextUsage;
    state->currentShaderStage = stage;
    if (IsWriteUsage(nextUsage)) {
      state->lastWriteStage = barrier.dstStage;
    } else {
      state->lastWriteStage = EPipelineStage::None;
    }

    return barrier;
  }

  void Clear() {
    m_textureStates.Clear();
    m_bufferStates.Clear();
  }

private:
  struct BufferState {
    EResourceUsage currentUsage = rhi::EResourceUsage::None;
    EPipelineStage lastWriteStage = rhi::EPipelineStage::None;
    EShaderStage currentShaderStage = EShaderStage::None;
  };
  struct TextureState {
    EResourceUsage currentUsage = rhi::EResourceUsage::None;
    EPipelineStage lastWriteStage = rhi::EPipelineStage::None;
    EResourceLayout currentLayout = rhi::EResourceLayout::Undefined;
    EShaderStage currentShaderStage = EShaderStage::None;
  };

  ImageBarrier CreateBarrier(TextureHandle handle, const TextureState &state,
                             EResourceUsage nextUsage, uint32_t layerCount,
                             uint32_t levelCount,
                             EShaderStage nextStage) const {

    EPipelineStage srcStage = state.lastWriteStage;
    if (srcStage == EPipelineStage::None) {
      srcStage =
          (state.currentLayout == EResourceLayout::Undefined)
              ? EPipelineStage::None
              : MapUsageToStage(state.currentUsage, state.currentShaderStage);
    }

    EAccess srcAccess = (srcStage == EPipelineStage::None)
                            ? EAccess::None
                            : MapUsageToAccess(state.currentUsage, true);

    return {
        .texture = handle,
        .oldLayout = state.currentLayout,
        .newLayout = MapUsageToLayout(nextUsage),
        .srcAccess = srcAccess,
        .dstAccess = MapUsageToAccess(nextUsage, true),
        .srcStage = srcStage,
        .dstStage = MapUsageToStage(nextUsage, nextStage),
        .levelCount = levelCount,
        .layerCount = layerCount,
    };
  }

  BufferBarrier CreateBarrier(BufferHandle handle, const BufferState &state,
                              EResourceUsage nextUsage, uint32_t offset,
                              uint32_t size, EShaderStage stage) const {
    EPipelineStage srcStage = state.lastWriteStage;
    if (srcStage == EPipelineStage::None) {
      srcStage = MapUsageToStage(state.currentUsage, state.currentShaderStage);
    }
    EAccess srcAccess = MapUsageToAccess(state.currentUsage, false);

    return {
        .buffer = handle,
        .srcAccess = srcAccess,
        .dstAccess = MapUsageToAccess(nextUsage, false),
        .srcStage = srcStage,
        .dstStage = MapUsageToStage(nextUsage, stage),
        .offset = offset,
        .size = size,
    };
  }

  HashMap<TextureHandle, TextureState> m_textureStates;
  HashMap<BufferHandle, BufferState> m_bufferStates;
};

} // namespace avalon::rhi
