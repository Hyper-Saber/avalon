module;
#include <cstddef>
#include <cstdint>
#include <cstring>
module avalon.core;
import :string;

namespace avalon {
void String::__ForceDebugSymbolExport() const {}

String::String(StringView view) { InitWith(view.GetData(), view.GetSize()); }

String::String(const String &other) {
  InitWith(other.GetData(), other.GetSize());
}

String::String(String &&other) noexcept {
  this->m_data.m_small = other.m_data.m_small;

  other.m_data.m_large.pData = nullptr;
  other.m_data.m_small.state = 0;
}

String::~String() { Release(); }

String &String::operator=(const String &other) {
  if (this == &other)
    return *this;

  Release();
  InitWith(other.GetData(), other.GetSize());
  return *this;
}

String &String::operator=(String &&other) noexcept {
  if (this == &other)
    return *this;

  Release();

  this->m_data.m_small = other.m_data.m_small;

  other.m_data.m_large.pData = nullptr;
  other.m_data.m_small.state = 0;
  return *this;
}

String &String::Append(StringView other) {
  const auto currentSize = GetSize();
  const auto otherSize = other.GetSize();

  if (otherSize == 0)
    return *this;

  const auto newSize = currentSize + otherSize;

  if (newSize <= kSSOCapacity) {
    std::memcpy(m_data.m_small.buffer + currentSize, other.GetData(),
                otherSize);
    m_data.m_small.buffer[newSize] = '\0';
    SetSmallState(static_cast<uint8_t>(newSize));
  } else {
    if (!IsLarge() || newSize > m_data.m_large.capacity) {
      auto newCapacity =
          IsLarge() ? m_data.m_large.capacity * 2 : kSSOCapacity * 2;
      if (newCapacity < newSize)
        newCapacity = newSize + 1;

      auto pNewMemory = static_cast<char *>(mem::Allocator<char>().Allocate(
          newCapacity + 1, mem::MemoryTag::String));
      std::memcpy(pNewMemory, GetData(), currentSize);
      std::memcpy(pNewMemory + currentSize, other.GetData(), otherSize);
      pNewMemory[newSize] = '\0';

      if (IsLarge()) {
        mem::Allocator<char>().Deallocate(m_data.m_large.pData,
                                          m_data.m_large.capacity + 1);
      }
      m_data.m_large.pData = pNewMemory;
      m_data.m_large.size = newSize;
      m_data.m_large.capacity = newCapacity;
      SetLargeFlag();

    } else {
      std::memcpy(m_data.m_large.pData + currentSize, other.GetData(),
                  otherSize);
      m_data.m_large.pData[newSize] = '\0';
      m_data.m_large.size = newSize;
    }
  }

  return *this;
}

String &String::Append(char c) {
  const auto currentSize = GetSize();
  const auto newSize = currentSize + 1;

  if (newSize <= kSSOCapacity) {
    m_data.m_small.buffer[currentSize] = c;
    m_data.m_small.buffer[newSize] = '\0';
    SetSmallState(static_cast<uint8_t>(newSize));
  } else {
    if (!IsLarge() || newSize > m_data.m_large.capacity) {
      auto newCapacity =
          IsLarge() ? m_data.m_large.capacity * 2 : kSSOCapacity * 2;
      if (newCapacity < newSize)
        newCapacity = newSize + 1;

      auto pNewMemory = static_cast<char *>(mem::Allocator<char>().Allocate(
          newCapacity + 1, mem::MemoryTag::String));
      std::memcpy(pNewMemory, GetData(), currentSize);
      pNewMemory[newSize - 1] = c;
      pNewMemory[newSize] = '\0';

      if (IsLarge()) {
        mem::Allocator<char>().Deallocate(m_data.m_large.pData,
                                          m_data.m_large.capacity + 1);
      }
      m_data.m_large.pData = pNewMemory;
      m_data.m_large.size = newSize;
      m_data.m_large.capacity = newCapacity;
      SetLargeFlag();

    } else {
      m_data.m_large.pData[currentSize] = c;
      m_data.m_large.pData[newSize] = '\0';
      m_data.m_large.size = newSize;
    }
  }

  return *this;
}

void String::InitWith(const char *pData, size_t size) {
  if (size <= kSSOCapacity) {
    std::memset(m_data.m_small.buffer, 0, kSSOCapacity);
    if (pData)
      std::memcpy(m_data.m_small.buffer, pData, size);
    m_data.m_small.buffer[size] = '\0';
    SetSmallState(static_cast<uint8_t>(size));

  } else {
    m_data.m_large.size = size;
    m_data.m_large.capacity = size + 1;
    m_data.m_large.pData = mem::Allocator<char>().Allocate(
        m_data.m_large.capacity + 1, mem::MemoryTag::String);
    if (pData)
      std::memcpy(m_data.m_large.pData, pData, size);
    m_data.m_large.pData[size] = '\0';
    SetLargeFlag();
  }
}

void String::Release() noexcept {
  if (IsLarge() && m_data.m_large.pData) {
    mem::Allocator<char>().Deallocate(m_data.m_large.pData,
                                      m_data.m_large.capacity + 1);
    m_data.m_large.pData = nullptr;
    m_data.m_small.state = 0;
  }
}
} // namespace avalon
