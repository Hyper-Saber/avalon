module;

export module avalon.core:vfs;
import :memory.blobs;
import :unique_ptr;
import :path;
import :context;

export namespace avalon::vfs {

enum class EVfsError {
  None,
  NotFound,
  AccessDenied,
  ReadError,
};

class IFileDevice : public IAutoDestroyable {
public:
  virtual ~IFileDevice() = default;
  virtual auto GetAbsolute(const Path &path, Path &outPath) const
      -> EVfsError = 0;
  virtual auto ReadFile(const Path &path, avalon::BlobPtr &outBlob) const
      -> EVfsError = 0;
  virtual auto IsExists(const Path &path) const -> bool = 0;
  virtual auto GetDeviceName() const -> StringView = 0;
};

class IVfs : public IAutoDestroyable {
public:
  virtual ~IVfs() = default;

  virtual void Mount(const String &virtualRoot, const Path &physicalRoot,
                     const IFileDevice *device, int priority = 0) = 0;
  virtual void Unmount(const String &virtualRoot) = 0;
  virtual auto GetAbsolute(const Path &path, Path &outPath) const
      -> EVfsError = 0;
  virtual auto ReadFile(const Path &path, BlobPtr &outBlob) -> EVfsError = 0;
  virtual auto IsExists(const Path &path) const -> bool = 0;
};

class VfsProvider {
public:
  static AVALON_CORE_API auto CreateVfs() -> UniquePtr<IVfs>;
  static AVALON_CORE_API auto CreateDevice() -> UniquePtr<IFileDevice>;
};

inline auto GetVfs() -> IVfs & {
  return GetContext().GetService<IVfs>(EEngineService::Vfs);
}

} // namespace avalon::vfs
