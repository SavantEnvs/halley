// The bundled ECS World is not part of the fuzzed subset; ConfigNode only touches it when
// SerializerOptions::world is set, which the harnesses never do. These satisfy the linker.
#include "halley/entity/entity.h"
#include "halley/entity/entity_id.h"
#include "halley/entity/world.h"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"

namespace Halley {
	String EntityId::toString() const { return Halley::toString(value); }
	Entity* World::tryGetRawEntity(EntityId) { return nullptr; }
	std::optional<EntityRef> World::findEntity(const UUID&) { return std::nullopt; }
	EntityId Entity::getEntityId() const { return EntityId(); }
}
