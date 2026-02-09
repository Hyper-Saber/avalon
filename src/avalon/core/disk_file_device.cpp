module;
#include <filesystem>
#include <fstream>
#include <string_view>
module avalon.core:disk_device;

import :vfs;
import :log;
import :memory.blobs;
import :containers.array;
import :platform;

namespace avalon::vfs {
constexpr std::string_view deviceName = "DiskDevice";
class DiskDevice final : public IFileDevice {
public:
  DiskDevice() = default;
  ~DiskDevice() override = default;

  auto ReadFile(const char *cPath, BlobPtr &outBlob) -> EVfsError override {
    auto path = ToPath(cPath);
    std::error_code errorCode;

    if (!std::filesystem::exists(path, errorCode) ||
        !std::filesystem::is_regular_file(path, errorCode)) {
      avalon::Error("[VFS]: file not found at path: {}", path.generic_string());
      return EVfsError::NotFound;
    }

    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
      avalon::Error("[VFS]: access denied at path: {}", path.generic_string());
      return EVfsError::AccessDenied;
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    Array<std::byte> buffer(fileSize);

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char *>(buffer.GetData()), fileSize)) {
      avalon::Error("[VFS]: read error at path: {}", path.generic_string());
      return EVfsError::ReadError;
    }

    outBlob = CreateBlob(std::move(buffer));
    return EVfsError::None;
  }

  bool IsPathExists(const char *path) const override {
    return std::filesystem::exists(path) &&
           std::filesystem::is_regular_file(path);
  }

  auto GetDeviceName() const -> const char * override {
    return deviceName.data();
  }
};

} // namespace avalon::vfs
