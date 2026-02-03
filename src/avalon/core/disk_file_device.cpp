module;
#include <expected>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>
module avalon.core:disk_device;

import :vfs;
import :memory;
import :log;

namespace avalon::vfs {
constexpr std::string_view deviceName = "DiskDevice";
class DiskDevice final : public IFileDevice {
public:
  DiskDevice() = default;
  ~DiskDevice() override = default;

  auto ReadFile(const std::filesystem::path &path)
      -> std::expected<BlobPtr, EVfsError> override {
    std::error_code errorCode;

    if (!std::filesystem::exists(path, errorCode) ||
        !std::filesystem::is_regular_file(path, errorCode)) {
      avalon::Error("[VFS]: file not found at path: {}", path.generic_string());
      return std::unexpected(EVfsError::NotFound);
    }

    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
      avalon::Error("[VFS]: access denied at path: {}", path.generic_string());
      return std::unexpected(EVfsError::AccessDenied);
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<std::byte> buffer(fileSize);

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), fileSize)) {
      avalon::Error("[VFS]: read error at path: {}", path.generic_string());
      return std::unexpected(EVfsError::ReadError);
    }
    return avalon::MakeBlob<avalon::VectorBlob>(std::move(buffer));
  }

  bool IsPathExists(const std::filesystem::path &path) const override {
    return std::filesystem::exists(path) &&
           std::filesystem::is_regular_file(path);
  }

  auto GetDeviceName() const -> std::string_view override { return deviceName; }
};

} // namespace avalon::vfs
