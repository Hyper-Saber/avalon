module;
#include <dlfcn.h>
module avalon.core;

import :string_view;

namespace avalon {

void *InternalLoadLibrary(StringView path) {
  return dlopen(path.GetData(), RTLD_NOW | RTLD_LOCAL);
}

void *InternalGetSymbol(void *handle, StringView symbol) {
  return dlsym(handle, symbol.GetData());
}
void InternalUnloadPlugin(void *handle) {
  Info("--- unloading Plugin Handle: {}", handle);
  if (handle)
    dlclose(handle);
}

} // namespace avalon
