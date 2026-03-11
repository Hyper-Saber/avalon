module;
export module avalon.engine:utils;

import avalon.core;

export namespace avalon::vfs {
auto FindProjectRoot(Path &startPath,
                     const StringView rootMarker = ".gitignore") {
  Path current;
  auto result = GetVfs().GetAbsolute(startPath, current);
  if (result == EVfsError::None) {
    while (!current.IsEmpty()) {
      auto probe = current / rootMarker;
      if (vfs::GetVfs().IsExists(probe)) {
        return current;
      }

      auto parent = current.GetParent();

      if (parent.GetId() == current.GetId() || parent.IsEmpty() ||
          parent.GetView() == ".") {
        break;
      }
      current = parent;
    }
  }
  return Path(".");
}

auto FindProjectRoot(const IFileDevice &device,
                     const StringView rootMarker = ".gitignore") {
  Path current;
  auto result = device.GetAbsolute(".", current);
  if (result == EVfsError::None) {
    while (!current.IsEmpty()) {
      auto probe = current / rootMarker;
      if (device.IsExists(probe)) {
        return current;
      }

      auto parent = current.GetParent();

      if (parent.GetId() == current.GetId() || parent.IsEmpty() ||
          parent.GetView() == ".") {
        break;
      }
      current = parent;
    }
  }
  return Path(".");
}

} // namespace avalon::vfs
