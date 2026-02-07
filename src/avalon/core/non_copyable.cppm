module;
export module avalon.core:non_copyable;

export namespace avalon {

class NonCopyable {
protected:
  constexpr NonCopyable() = default;
  ~NonCopyable() = default;

public:
  NonCopyable(const NonCopyable &) = delete;
  NonCopyable &operator=(const NonCopyable &) = delete;
  NonCopyable(NonCopyable &&) = delete;
  NonCopyable &operator=(NonCopyable &&) = delete;
};
} // namespace avalon
