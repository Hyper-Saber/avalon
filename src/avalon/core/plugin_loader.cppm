module;
#include <expected>
#include <memory>
#include <string_view>
export module avalon.core:plugin_loader;
import :status;
import :log;

export namespace avalon {

class IPlugin {
public:
  virtual ~IPlugin() = default;
  virtual auto OnLoad() -> EStatusCode = 0;
};

AVALON_CORE_API void *InternalLoadLibrary(std::string_view path);
AVALON_CORE_API void *InternalGetSymbol(void *handle, std::string_view symbol);
AVALON_CORE_API void InternalUnloadPlugin(void *handle);

template <typename T>
concept TPlugin = std::derived_from<T, IPlugin>;

template <TPlugin T> class PluginInstance {
public:
  struct Deleter {
    using DestroyFunc = void (*)(T *);
    void *handle;
    DestroyFunc destroyFn;

    void operator()(T *ptr) const {
      if (ptr && destroyFn) {
        destroyFn(ptr);
      }

      if (handle) {
        InternalUnloadPlugin(handle);
      }
    }
  };

  PluginInstance(T *instance, void *handle,
                 typename Deleter::DestroyFunc destroyFn)
      : m_instance(instance,
                   Deleter{.handle = handle, .destroyFn = destroyFn}) {}

  PluginInstance(const PluginInstance &) = delete;
  PluginInstance &operator=(const PluginInstance &) = delete;
  PluginInstance(PluginInstance &&) = default;
  PluginInstance &operator=(PluginInstance &&) = default;

  T *operator->() { return m_instance.get(); }
  const T *operator->() const { return m_instance.get(); }
  T *get() { return m_instance.get(); }
  const T *get() const { return m_instance.get(); }

private:
  std::unique_ptr<T, Deleter> m_instance;
};

template <TPlugin T>
inline auto LoadPlugin(std::string_view path,
                       std::string_view entry = "CreatePlugin",
                       std::string_view exit = "DestroyPlugin")
    -> std::expected<PluginInstance<T>, EStatusCode> {

  void *handle = InternalLoadLibrary(path);
  if (!handle) {
    avalon::Error("Plugin Loader: Plugin not found! Path: {}.", path);
    return std::unexpected(EStatusCode::FileNotFound);
  }

  void *symbolCreate = InternalGetSymbol(handle, entry);
  void *symbolDestroy = InternalGetSymbol(handle, exit);
  if (!symbolCreate || !symbolDestroy) {
    avalon::Error("Plugin Loader: Plugin entry/exit point not found! Path: {}, "
                  "entry: {}, exit: {}.",
                  path, entry, exit);
    InternalUnloadPlugin(handle);
    return std::unexpected(EStatusCode::SymbolNotFound);
  }

  using CreateFunc = T *(*)();
  using DestroyFunc = PluginInstance<T>::Deleter::DestroyFunc;
  auto createFn = reinterpret_cast<CreateFunc>(symbolCreate);
  auto destroyFn = reinterpret_cast<DestroyFunc>(symbolDestroy);

  if (!createFn || !destroyFn) {
    avalon::Error("Plugin Loader: Plugin entry/exit point not found! Path: {}, "
                  "entry: {}, exit: {}.",
                  path, entry, exit);
    InternalUnloadPlugin(handle);
    return std::unexpected(EStatusCode::SymbolNotFound);
  }

  T *raw_instance = createFn();
  if (!raw_instance) {
    avalon::Error("Plugin Loader: Failed to create plugin instance!");
    InternalUnloadPlugin(handle);
    return std::unexpected(EStatusCode::PluginInitializeError);
  }

  auto res = raw_instance->OnLoad();
  if (res != EStatusCode::Success) {
    avalon::Error("Plugin Loader: Failed to initialize! Path: {}", path);
    destroyFn(raw_instance);
    InternalUnloadPlugin(handle);
    return std::unexpected(res);
  }

  return PluginInstance<T>(raw_instance, handle, destroyFn);
}

} // namespace avalon
