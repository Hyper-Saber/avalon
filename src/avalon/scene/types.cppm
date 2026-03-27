module;
#include <cstdint>
export module avalon.scene:types;
import avalon.core;
import avalon.graphics;

export namespace avalon::scene {
enum class EProjectionType {
  Perspective,
};

enum class ELightType : uint32_t {
  Directional,
  Spot,
};
} // namespace avalon::scene
