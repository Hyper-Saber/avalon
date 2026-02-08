module;
#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

module avalon.core;
import :vfs;
import :memory;
import :memory.blobs;
import :disk_device;

namespace avalon::vfs {

struct MountEntry {
  std::string_view virtualRoot;
  std::filesystem::path physicalRoot;
  IFileDevice *device;
  int priority;

  bool operator>(const MountEntry &other) const {
    return priority > other.priority;
  }
};

class Vfs final : public IVfs {
public:
  void Mount(const std::string_view virtualRoot,
             const std::filesystem::path &physicalRoot, IFileDevice *fileDevice,
             int priority) override {
    m_mounts.push_back({virtualRoot, physicalRoot, fileDevice, priority});

    std::sort(m_mounts.begin(), m_mounts.end(), std::greater<MountEntry>());
  }

  void Unmount(const std::string_view virtualRoot) override {
    std::erase_if(m_mounts, [&](const auto &entry) {
      return entry.virtualRoot == virtualRoot;
    });
  }

  auto ReadFile(const std::filesystem::path &path)
      -> std::expected<avalon::BlobPtr, EVfsError> override {
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      if (device->IsPathExists(targetPath)) {
        return device->ReadFile(targetPath);
      }
    }

    return std::unexpected(EVfsError::NotFound);
  }

  auto IsExists(const std::filesystem::path &path) const -> bool override {
    if (auto resolved = ResolvePath(path)) {
      auto [device, targetPath] = *resolved;
      return device->IsPathExists(targetPath);
    }
    return false;
  }

private:
  std::vector<MountEntry> m_mounts;

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

        auto targetPath = mount.physicalRoot / relative;
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
