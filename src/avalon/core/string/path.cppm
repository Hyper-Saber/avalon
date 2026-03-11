module;
#include <format>
#include <string_view>

export module avalon.core:path;

import :string;
import :string_view;

export namespace avalon {
class Path {
public:
  constexpr Path() noexcept = default;
  Path(StringView path) : m_path(path) { Normalize(); }
  Path(const char *path) : m_path(StringView(path)) { Normalize(); }

  Path operator/(StringView subPath) const {
    if (subPath.IsEmpty())
      return *this;
    if (m_path.IsEmpty())
      return Path(subPath);

    String result = m_path;

    bool leftHasSlash = static_cast<StringView>(result).GetBack() == '/';
    bool rightHasSlash =
        subPath.GetFront() == '/' || subPath.GetFront() == '\\';
    if (leftHasSlash && rightHasSlash) {
      result += subPath.GetSubView(1);
    } else if (!leftHasSlash && !rightHasSlash) {
      result += '/';
      result += subPath;
    } else {
      result += subPath;
    }

    return Path(result);
  }

  Path operator/(const Path &subPath) const {
    return *this / subPath.GetView();
  }

  StringId GetId() const noexcept { return StringId(GetView()); }
  StringView GetView() const noexcept {
    return static_cast<StringView>(m_path);
  }

  auto GetString() const noexcept -> const String & { return m_path; }

  const char *GetCStr() const noexcept { return m_path.GetData(); }
  bool IsEmpty() const noexcept { return m_path.IsEmpty(); }

  StringView GetExtension() const noexcept {
    auto view = GetView();
    auto pos = FindLastOf(view, '.');
    if (pos == StringView::kNpos || pos == view.GetSize() - 1)
      return {};
    return StringView(view.GetData() + pos, view.GetSize() - pos);
  }

  StringView GetFileName() const noexcept {
    auto view = GetView();
    auto pos = FindLastOf(view, '/');
    if (pos == StringView::kNpos)
      return view;
    return StringView(view.GetData() + pos + 1, view.GetSize() - pos - 1);
  }

  Path GetParent() const noexcept {
    auto view = GetView();
    auto pos = FindLastOf(view, '/');
    if (pos == StringView::kNpos)
      return Path(".");
    return Path(StringView(view.GetData(), pos));
  }

  bool IsStartWith(StringView view) const noexcept {
    auto current = GetView();
    if (view.IsEmpty())
      return true;
    if (view.GetSize() > current.GetSize())
      return false;

    auto prefix = StringView(current.GetData(), view.GetSize());
    if (prefix != view)
      return false;

    if (current.GetSize() == view.GetSize())
      return true;

    char nextChar = current.GetData()[view.GetSize() - 1];
    if (nextChar == '/')
      return true;

    return current.GetData()[view.GetSize()] == '/';
  }

  Path RemovePrefix(StringView view) const noexcept {
    if (!IsStartWith(view))
      return *this;

    auto current = GetView();
    auto skipLen = view.GetSize();

    if (skipLen < current.GetSize() && current.GetData()[skipLen] == '/') {
      skipLen++;
    }
    auto remaining = current.GetSubView(skipLen);
    if (remaining.IsEmpty())
      return Path(".");

    return Path(remaining);
  }

private:
  String m_path;

  void Normalize() {
    auto pData = const_cast<char *>(m_path.GetData());
    auto size = m_path.GetSize();
    for (size_t i = 0; i < size; i++) {
      if (pData[i] == '\\')
        pData[i] = '/';
    }
  }

  static size_t FindLastOf(StringView view, char c) {
    auto pData = view.GetData();
    for (size_t i = view.GetSize(); i > 0; --i) {
      if (pData[i - 1] == c)
        return i - 1;
    }
    return StringView::kNpos;
  }
};
} // namespace avalon

export namespace std {
template <> struct formatter<avalon::Path> : formatter<std::string_view> {
  auto format(const avalon::Path &path, format_context &ctx) const {
    return formatter<std::string_view, char>::format(
        std::string_view(path.GetCStr(), path.GetView().GetSize()), ctx);
  };
};
} // namespace std
