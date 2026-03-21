module avalon.core;

import :string_id;
import :string;
import :string_registry;
import :hash;
namespace avalon {
String StringId::Resolve() const { return ResolveStringId(m_id); }
} // namespace avalon
