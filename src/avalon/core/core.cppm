module;
#include <string_view>
export module avalon.core;
export import :log;
export import :status;
export import :types;
export import :plugin_loader;

namespace avalon {
export constexpr std::string_view kPluginExtension = AVALON_PLATFORM_DL_EXT;
}
