#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/variant.hpp"

using namespace godot;

class RoadManager : public RefCounted {
	GDCLASS(RoadManager, RefCounted)

protected:
	static void _bind_methods();

public:
	RoadManager() = default;
	~RoadManager() override = default;

	void print_type(const Variant &p_variant) const;
};
