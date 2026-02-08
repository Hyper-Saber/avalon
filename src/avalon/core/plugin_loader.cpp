module;
#include <dlfcn.h>
#include <string_view>
module avalon.core;

namespace avalon {

void *InternalLoadLibrary(std::string_view path) {
  return dlopen(path.data(), RTLD_NOW | RTLD_LOCAL);
}

void *InternalGetSymbol(void *handle, std::string_view symbol) {
  return dlsym(handle, symbol.data());
}
void InternalUnloadPlugin(void *handle) {
  Info("--- unloading Plugin Handle: {}", handle);
  if (handle)
    dlclose(handle);
}

} // namespace avalon
