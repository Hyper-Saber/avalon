module;
#include <string_view>
export module avalon.core;
export import :log;
export import :status;
export import :types;
export import :plugin_loader;
export import :vfs;
export import :memory;
export import :platform;
export import :ref_counted;

namespace avalon {
export constexpr std::string_view kPluginExtension = AVALON_PLATFORM_DL_EXT;
}
