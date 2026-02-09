module;
#include <filesystem>

module avalon.core;
import :vfs;
import :memory;
import :memory.blobs;
import :disk_device;
import :containers;

namespace avalon::vfs {

struct MountEntry {
  std::string virtualRoot;
  std::filesystem::path physicalPath;
  IFileDevice *device;
  int priority;

  bool operator>(const MountEntry &other) const {
    return priority > other.priority;
  }
};

class Vfs final : public IVfs {
public:
  void Mount(const char *virtualRoot, const char *cPhysicalPath,
             IFileDevice *fileDevice, int priority) override {
    m_mounts.PushBack(
        {virtualRoot, ToPath(cPhysicalPath), fileDevice, priority});

    std::sort(m_mounts.begin(), m_mounts.end(), std::greater<MountEntry>());
  }

  void Unmount(const char *virtualRoot) override {
    m_mounts.EraseIf(
        [&](const auto &entry) { return entry.virtualRoot == virtualRoot; });
  }

  auto ReadFile(const char *cPath, BlobPtr &outBlob) -> EVfsError override {
    auto path = ToPath(cPath);
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      auto cTargetPath = targetPath.c_str();
      if (device->IsPathExists(cTargetPath)) {
        return device->ReadFile(cTargetPath, outBlob);
      }
    }

    return EVfsError::NotFound;
  }

  auto IsExists(const char *path) const -> bool override {
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      return device->IsPathExists(targetPath.c_str());
    }
    return false;
  }

private:
  Array<MountEntry> m_mounts;

  auto ResolvePath(const std::filesystem::path &path) const
      -> std::optional<std::pair<IFileDevice *, std::filesystem::path>> {
    auto pathStr = path.generic_string();
    for (const auto &mount : m_mounts) {
      if (pathStr.starts_with(mount.virtualRoot)) {
        std::string_view relative = pathStr;
        relative.remove_prefix(mount.virtualRoot.length());

        while (!relative.empty() &&
               (relative[0] == '/' || relative[0] == '\\')) {
          relative.remove_prefix(1);
        }

        auto targetPath = mount.physicalPath / relative;
        return std::make_pair(mount.device, targetPath);
      }
    }
    return std::nullopt;
  }
};

auto VfsProvider::CreateVfs() -> std::unique_ptr<IVfs> {
  return std::make_unique<Vfs>();
}

auto VfsProvider::CreateDevice() -> std::unique_ptr<IFileDevice> {
  return std::make_unique<DiskDevice>();
}

} // namespace avalon::vfs
