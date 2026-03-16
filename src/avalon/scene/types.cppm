module;
#include <cstdint>
export module avalon.scene:types;

export namespace avalon::scene {
enum class EProjectionType {
  Perspective,
};

enum class ELightType : uint32_t {
  Directional,
};
} // namespace avalon::scene
