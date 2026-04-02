#ifndef INTERSECTION_NODE_H
#define INTERSECTION_NODE_H

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/path3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {

struct JunctionInfo {
	Path3D *path = nullptr;
	int point = 0;
};

struct JunctionEdge {
	Vector3 left;
	Vector3 right;

	Vector3 center() const { return (left + right) * 0.5f; }

	JunctionEdge() {}
	JunctionEdge(const Vector3 &p_left, const Vector3 &p_right) : left(p_left), right(p_right) {}
};

class Intersection {
public:
	Vector<JunctionInfo> junctions;

	void add_junction(Path3D *p_path, int p_point) {
		JunctionInfo info;
		info.path = p_path;
		info.point = p_point;
		junctions.push_back(info);
	}

	void clear() {
		junctions.clear();
	}

	const Vector<JunctionInfo> &get_junctions() const {
		return junctions;
	}
};

class IntersectionNode : public Node3D {
	GDCLASS(IntersectionNode, Node3D)

private:
	Transform3D last_transform;

	Intersection intersection;

	TypedArray<NodePath> paths;
	PackedInt32Array point_path;
	PackedFloat32Array curves;
	Ref<StandardMaterial3D> intersection_material;

	Vector3 center;
	Vector<Vector3> curve_points;

	MeshInstance3D *intersection_mesh = nullptr;
	Ref<ArrayMesh> intersection_array_mesh;

	Vector3 evaluate_position(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, float t) const;
	int sort_points(const Vector3 &p_center, const Vector3 &point1, const Vector3 &point2) const;
	void on_build_junction();
	void get_verts();
	void build_mesh();

	bool show_debug = false;
	Vector<MeshInstance3D *> debug_instances;

	void clear_debug_boxes();
	void debug_boxes();
	void refresh_debug_boxes();

protected:
	static void _bind_methods();

public:
	IntersectionNode();
	~IntersectionNode();

	void _ready() override;
	void _process(double p_delta) override;

	void set_paths(const TypedArray<NodePath> &p_paths);
	TypedArray<NodePath> get_paths() const;

	void set_point_path(const PackedInt32Array &p_point_path);
	PackedInt32Array get_point_path() const;

	void set_curves(const PackedFloat32Array &p_curves);
	PackedFloat32Array get_curves() const;

	void set_intersection_material(const Ref<StandardMaterial3D> &p_material);
	Ref<StandardMaterial3D> get_intersection_material() const;

	void set_show_debug(bool p_show_debug);
	bool get_show_debug() const;
};

} // namespace godot

#endif // INTERSECTION_NODE_H