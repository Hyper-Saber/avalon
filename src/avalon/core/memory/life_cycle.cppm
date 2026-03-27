module;
#include <debug/assert.hpp>
#include <utility>
export module avalon.core:life_cycle;

import :memory;
import :debug;

namespace avalon::mem {
class LifeCycle final {
public:
  template <TAutoDestroyable T> static bool Initialize(T *instance) {
    /**
     * @brief 激活对象的生命周期 (Created -> Initialized)
     * 作用：触发对象的 Initialize()
     * 逻辑，将其状态从“已分配但未配置”提升为“已就绪”。
     * @return 成功返回 true，失败返回 false。
     */
    if (!instance)
      return false;

    if (instance->m_lifecycleState != IAutoDestroyable::State::Created)
      return instance->m_lifecycleState == IAutoDestroyable::State::Initialized;

    if (instance->Initialize()) {
      instance->MarkInitialized();
      return true;
    }

    return false;
  }

  /**
   * @brief 在指定的内存地址上实例化对象 (Placement New + Initialization)
   * * 作用：
   * 1. 使用 placement new 在指定地址构造 T 类型对象。
   * 2. 若 T 满足 TAutoDestroyable，则自动调用 LifeCycle::Initialize()
   * 将其状态从 Created 推进至 Initialized。
   * * 注意：
   * - 此方法仅负责对象的构造与逻辑初始化，不涉及内存分配（由调用者提供地址）。
   * - 若初始化失败，将触发析构函数并返回 nullptr，防止“半成品”对象泄露。
   * * @param address 目标内存地址，需确保空间足够且对齐。
   * @param args 构造函数参数。
   * @return 成功则返回对象指针，否则返回 nullptr。
   */
  template <typename T, typename... Args>
  static T *Instantiate(void *address, Args &&...args) {
    T *instance = new (address) T(std::forward<Args>(args)...);

    if constexpr (TAutoDestroyable<T>) {
      bool isSuccess = Initialize(instance);
      AVALON_ASSERT_MSG(isSuccess, "[LifeCycle]: Failed to initialize object!");
      if (!isSuccess) {
        instance->~T();
        return nullptr;
      }
    }

    return instance;
  }

  /**
   * @brief 彻底销毁对象 (Initialized -> Destroyed)
   * 作用：对象析构 (~T) + 物理内存归还
   */
  template <TAutoDestroyable T> static void Destroy(T *instance) {
    if (instance &&
        instance->m_lifecycleState == IAutoDestroyable::State::Initialized) {
      instance->MarkInvalidated();
      instance->Destroy();
    }
  }

  /**
   * @brief 解除对象实例化状态 (Initialized -> Destroyed)
   * 作用：触发析构 (~T)，但不执行物理内存归还 (No Deallocate)。
   */
  template <typename T> static void Deinstantiate(T *instance) {
    if (!instance)
      return;
    if constexpr (TAutoDestroyable<T>) {
      if (instance->m_lifecycleState == IAutoDestroyable::State::Initialized) {
        instance->MarkInvalidated();
        instance->~T();
      }
    } else if constexpr (!std::is_trivially_destructible_v<T>) {
      instance->~T();
    }
  }
};
} // namespace avalon::mem
