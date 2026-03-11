module;
#include <utility>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

module avalon.core:vfs.disk_device;

import :string_view;
import :log;
import :vfs;

namespace avalon::vfs {
constexpr StringView deviceName = "DiskDevice";
class DiskDevice final : public IFileDevice {
public:
  DiskDevice() = default;
  ~DiskDevice() override = default;

  void Destroy() noexcept override {
    this->~DiskDevice();
    mem::Allocator<DiskDevice> alloc;
    alloc.Deallocate(this, 1);
  }

  auto GetAbsolute(const Path &path, Path &outPath) const
      -> EVfsError override {
    const char *cPath = path.GetCStr();
    if (!cPath || path.IsEmpty())
      return EVfsError::NotFound;

#ifdef __linux__
    char *resolved = realpath(cPath, nullptr);
    if (resolved) {
      outPath = Path(resolved);
      free(resolved);
    } else {
      Error("[VFS]: Could not resolve realpath for: {}.", path);
      return EVfsError::NotFound;
    }
#else
#endif
    return EVfsError::None;
  }

  auto ReadFile(const Path &path, BlobPtr &outBlob) const
      -> EVfsError override {
    auto cPath = path.GetCStr();
#ifdef __linux__
    int file = open(cPath, O_RDONLY | O_CLOEXEC);
    if (file == -1) {
      avalon::Error("[VFS]: file not found at path: {}", path);
      return EVfsError::NotFound;
    }

    struct stat st;
    if (fstat(file, &st) == -1) {
      close(file);
      avalon::Error("[VFS]: stat error at path: {}", path);
      return EVfsError::ReadError;
    }
    auto size = static_cast<size_t>(st.st_size);
    if (size == 0) {
      close(file);
      return EVfsError::None;
    }
    Array<std::byte> buffer(size);
    if (!buffer.GetData()) {
      close(file);
      return EVfsError::ReadError;
    }

    size_t totalRead = 0;
    while (totalRead < size) {
      ssize_t bytesRead =
          read(file, buffer.GetData() + totalRead, size - totalRead);
      if (bytesRead == -1) {
        if (errno == EINTR)
          continue;
        avalon::Error("[VFS]: read error at path: {}", path);
        close(file);
        return EVfsError::ReadError;
      } else if (bytesRead == 0)
        break;
      totalRead += bytesRead;
    }

    close(file);

    if (totalRead != size) {
      avalon::Error("[VFS]: read error at path: {}", path);
      return EVfsError::ReadError;
    }
#else
#endif

    outBlob = CreateBlob(std::move(buffer));
    return EVfsError::None;
  }

  bool IsExists(const Path &path) const override {
    auto cstr = path.GetCStr();
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.GetCStr());
    return (attrs != INVALID_FILE_ATTRIBUTES);
#else
    struct stat st;
    return (stat(cstr, &st) == 0);
#endif // _WIN32
  }

  auto GetDeviceName() const -> StringView override { return deviceName; }
};

} // namespace avalon::vfs
