module;
export module avalon.core:string_registry;
import :hash;

namespace avalon {
export class String;
export class StringView;

void RegisterStringId(HashType id, StringView str) noexcept;
String ResolveStringId(HashType id) noexcept;

} // namespace avalon
