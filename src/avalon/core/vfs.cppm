module;
#include <bits/unique_ptr.h>

export module avalon.core:vfs;
import :memory;
import :memory.blobs;

namespace avalon {
class Engine;
}

export namespace avalon::vfs {

enum class EVfsError {
  None,
  NotFound,
  AccessDenied,
  ReadError,
};

class IFileDevice {
public:
  virtual ~IFileDevice() = default;
  virtual auto ReadFile(const char *path, avalon::BlobPtr &outBlob)
      -> EVfsError = 0;
  virtual auto IsPathExists(const char *path) const -> bool = 0;
  virtual auto GetDeviceName() const -> const char * = 0;
};

class IVfs {
public:
  virtual ~IVfs() = default;

  virtual void Mount(const char *virtualRoot, const char *physicalRoot,
                     IFileDevice *device, int priority = 0) = 0;
  virtual void Unmount(const char *virtualRoot) = 0;
  virtual auto ReadFile(const char *path, BlobPtr &outBlob) -> EVfsError = 0;
  virtual auto IsExists(const char *path) const -> bool = 0;
};
class VfsProvider {
public:
  static AVALON_CORE_API auto CreateVfs() -> std::unique_ptr<IVfs>;
  static AVALON_CORE_API auto CreateDevice() -> std::unique_ptr<IFileDevice>;

private:
  VfsProvider() = default;
  friend class avalon::Engine;
  friend class VfsTestAccess;
};

class VfsTestAccess {
public:
  static auto Create() { return VfsProvider::CreateVfs(); }
  static auto CreateDevice() { return VfsProvider::CreateDevice(); }
};
} // namespace avalon::vfs
