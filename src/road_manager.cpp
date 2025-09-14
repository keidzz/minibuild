#include "road_manager.h"

void RoadManager::_bind_methods() {
	godot::ClassDB::bind_method(D_METHOD("print_type", "variant"), &RoadManager::print_type);
}

void RoadManager::print_type(const Variant &p_variant) const {
	print_line(vformat("Type: %d", p_variant.get_type()));
}
