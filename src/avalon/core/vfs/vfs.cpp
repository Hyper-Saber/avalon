module;
#include <algorithm>
#include <debug/assert.hpp>
#include <functional>
#include <optional>

module avalon.core;

import :vfs.disk_device;
import :vfs;
import :unique_ptr;
import :debug;

namespace avalon::vfs {

struct MountEntry {
  String virtualRoot;
  Path physicalPath;
  const IFileDevice *device;
  int priority;

  MountEntry(const String &virtualRoot, const Path &physicalPath,
             const IFileDevice *device, int priority)
      : virtualRoot(virtualRoot), physicalPath(physicalPath), device(device),
        priority(priority) {}

  bool operator>(const MountEntry &other) const {
    return priority > other.priority;
  }
};

void HandleError(EVfsError err, const Path &path) {
  switch (err) {
  case EVfsError::None:
    break;
  case EVfsError::NotFound:
    Error("[VFS]: File not found at path:{}", path);
    break;
  case EVfsError::AccessDenied:
    Error("[VFS]: Access denied at path:{}", path);
    break;
  case EVfsError::ReadError:
    Error("[VFS]: Read error at path:{}", path);
    break;
  }
}

class Vfs final : public IVfs {
public:
  void Destroy() noexcept override {
    this->~Vfs();
    mem::Allocator<Vfs> alloc;
    alloc.Deallocate(this, 1);
  }

  void Mount(const String &virtualRoot, const Path &physicalPath,
             const IFileDevice *fileDevice, int priority) override {

    AVALON_ASSERT_MSG(!virtualRoot.IsEmpty(), "[VFS]: VirtualRoot is Empty!");

    String normalizedRoot = virtualRoot;
    if (normalizedRoot.GetSize() > 0 &&
        normalizedRoot.GetData()[normalizedRoot.GetSize() - 1] != '/') {
      normalizedRoot += '/';
    }
    m_mounts.PushBack({normalizedRoot, physicalPath, fileDevice, priority});

    std::sort(m_mounts.begin(), m_mounts.end(), std::greater<MountEntry>());
  }

  void Unmount(const String &virtualRoot) override {
    m_mounts.EraseIf(
        [&](const auto &entry) { return entry.virtualRoot == virtualRoot; });
  }

  auto GetAbsolute(const Path &path, Path &outPath) const
      -> EVfsError override {
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      auto cTargetPath = targetPath.GetCStr();
      if (device->IsExists(cTargetPath)) {
        auto err = device->GetAbsolute(cTargetPath, outPath);
        if (err != EVfsError::None) {
          HandleError(err, path);
          return err;
        }
        return EVfsError::None;
      }
    }

    return EVfsError::NotFound;
  }

  auto ReadFile(const Path &path, BlobPtr &outBlob) -> EVfsError override {
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      if (device->IsExists(targetPath)) {
        auto err = device->ReadFile(targetPath, outBlob);
        if (err != EVfsError::None) {
          HandleError(err, path);
          return err;
        }
        return EVfsError::None;
      }
      Error("[VFS]: File not found at path: {}", targetPath);
    } else {
      Error("[VFS]: Failed to resolve path: {}", path);
    }

    return EVfsError::NotFound;
  }

  auto IsExists(const Path &path) const -> bool override {
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      return device->IsExists(targetPath);
    }
    return false;
  }

private:
  Array<MountEntry> m_mounts;

  auto ResolvePath(const Path &path) const
      -> std::optional<std::pair<const IFileDevice *, Path>> {
    for (const auto &mount : m_mounts) {
      if (path.IsStartWith(mount.virtualRoot)) {
        auto relative = path.RemovePrefix(mount.virtualRoot);

        auto targetPath = mount.physicalPath / relative;
        return std::make_pair(mount.device, targetPath);
      }
    }
    return std::nullopt;
  }
};

auto VfsProvider::CreateVfs() -> UniquePtr<IVfs> { return MakeUnique<Vfs>(); }

auto VfsProvider::CreateDevice() -> UniquePtr<IFileDevice> {
  return MakeUnique<DiskDevice>();
}

} // namespace avalon::vfs
