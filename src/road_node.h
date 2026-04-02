#pragma once

#include <godot_cpp/classes/path3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/classes/box_mesh.hpp>


namespace godot {

class RoadNode : public Path3D {
	GDCLASS(RoadNode, Path3D)

private:
	bool show_debug = false;
	Vector<MeshInstance3D *> debug_instances;

	void clear_debug_boxes();
	void debug_boxes();
	void refresh_debug_boxes();

	int resolution = 80;
	float width = 1.0f;
	Ref<StandardMaterial3D> road_material;

	Vector<Vector3> m_verts_p1;
	Vector<Vector3> m_verts_p2;

	MeshInstance3D *road_mesh = nullptr;
	Ref<ArrayMesh> road_array_mesh;

	void _on_curve_changed();
	void build_mesh();

protected:
	static void _bind_methods();

public:
	RoadNode();
	~RoadNode();

	void _ready() override;

	void set_resolution(int p_resolution);
	int get_resolution() const;

	void set_width(float p_width);
	float get_width() const;

	void set_road_material(const Ref<StandardMaterial3D> &p_material);
	Ref<StandardMaterial3D> get_road_material() const;

	void set_show_debug(bool p_show_debug);
	bool get_show_debug() const;
};

} // namespace godot