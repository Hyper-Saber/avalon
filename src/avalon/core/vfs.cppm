module;
#include <expected>
#include <filesystem>
#include <string_view>

export module avalon.core:vfs;
import :memory;

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
  virtual auto ReadFile(const std::filesystem::path &path)
      -> std::expected<avalon::BlobPtr, EVfsError> = 0;
  virtual auto IsPathExists(const std::filesystem::path &path) const
      -> bool = 0;
  virtual auto GetDeviceName() const -> std::string_view = 0;
};

class IVfs {
public:
  virtual ~IVfs() = default;

  virtual void Mount(const std::string_view virtualRoot,
                     const std::filesystem::path &physicalRoot,
                     IFileDevice *device, int priority = 0) = 0;
  virtual void Unmount(const std::string_view virtualRoot) = 0;

  virtual auto ReadFile(const std::filesystem::path &path)
      -> std::expected<avalon::BlobPtr, EVfsError> = 0;
  virtual auto IsExists(const std::filesystem::path &path) const -> bool = 0;
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
