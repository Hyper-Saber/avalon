module;
#include <cstdint>
#include <utility>
export module avalon.core:context;
import :types;
import :memory;
import :unique_ptr;

export namespace avalon {

enum class EEngineService : uint32_t {
  Vfs = 0,
  ShaderManager,
  MeshManager,
  MaterialManager,
  Count
};

class EngineContext {
public:
  rhi::RenderBackend rhi;

  template <TAutoDestroyable T>
  auto GetService(EEngineService service) const noexcept -> T & {
    auto *pBase = services[static_cast<uint32_t>(service)].Get();
    return *reinterpret_cast<T *>(pBase);
  }

  template <TAutoDestroyable T, typename... Args>
  auto RegisterService(EEngineService service, Args &&...args) {
    services[static_cast<uint32_t>(service)] =
        MakeUnique<T>(std::forward<Args>(args)...);
  }

  template <TAutoDestroyable T>
  auto RegisterService(EEngineService serviceType, UniquePtr<T> service) {
    services[static_cast<uint32_t>(serviceType)] = std::move(service);
  }

private:
  UniquePtr<mem::IAutoDestroyable>
      services[static_cast<uint32_t>(EEngineService::Count)];
};

inline auto GetContext() -> EngineContext & {
  static EngineContext instance;
  return instance;
}
} // namespace avalon
