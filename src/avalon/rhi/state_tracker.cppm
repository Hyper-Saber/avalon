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
                   EResourceUsage nextUsage, uint32_t layerCount)
      -> std::optional<ImageBarrier> {

    auto state = m_resourceState.Get(texture);

    if (!state) {
      ResourceState initialState{.currentUsage = EResourceUsage::None,
                                 .currentLayout = EResourceLayout::Undefined,
                                 .lastWriteStage = EPipelineStage::None};

      auto barrier =
          CreateBarrier(texture, initialState, nextUsage, layerCount);

      m_resourceState.Insert(texture,
                             {.currentUsage = nextUsage,
                              .currentLayout = barrier.newLayout,
                              .lastWriteStage = IsWriteUsage(nextUsage)
                                                    ? barrier.dstStage
                                                    : EPipelineStage::None});
      return barrier;
    }

    auto nextLayout = MapUsageToLayout(nextUsage);

    if (state->currentLayout == nextLayout &&
        !IsWriteUsage(state->currentUsage) && !IsWriteUsage(nextUsage)) {
      return std::nullopt;
    }

    auto barrier = CreateBarrier(texture, *state, nextUsage, layerCount);

    state->currentUsage = nextUsage;
    state->currentLayout = barrier.newLayout;
    if (IsWriteUsage(nextUsage)) {
      state->lastWriteStage = barrier.dstStage;
    } else if (barrier.newLayout != state->currentLayout) {
      state->lastWriteStage = EPipelineStage::None;
    }

    return barrier;
  }

  void Clear() { m_resourceState.Clear(); }

private:
  ImageBarrier CreateBarrier(TextureHandle handle, const ResourceState &state,
                             EResourceUsage nextUsage,
                             uint32_t layerCount) const {

    EPipelineStage srcStage = state.lastWriteStage;
    if (srcStage == EPipelineStage::None) {
      srcStage = (state.currentLayout == EResourceLayout::Undefined)
                     ? EPipelineStage::None
                     : MapUsageToStage(state.currentUsage);
    }

    EAccess srcAccess = (srcStage == EPipelineStage::None)
                            ? EAccess::None
                            : MapUsageToAccess(state.currentUsage);

    return {
        .texture = handle,
        .oldLayout = state.currentLayout,
        .newLayout = MapUsageToLayout(nextUsage),
        .srcAccess = srcAccess,
        .dstAccess = MapUsageToAccess(nextUsage),
        .srcStage = srcStage,
        .dstStage = MapUsageToStage(nextUsage),
        .layerCount = layerCount,
    };
  }

  HashMap<TextureHandle, ResourceState> m_resourceState;
};

} // namespace avalon::rhi
