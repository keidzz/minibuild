#include "intersection_node.h"
#include "road_node.h"
#include "road_tools.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <vector>

namespace godot {

IntersectionNode::IntersectionNode() {
}

IntersectionNode::~IntersectionNode() {
}

void IntersectionNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_paths", "paths"), &IntersectionNode::set_paths);
	ClassDB::bind_method(D_METHOD("get_paths"), &IntersectionNode::get_paths);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "paths", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::NODE_PATH) + "/" + String::num_int64(PROPERTY_HINT_NODE_PATH_VALID_TYPES) + ":Path3D"), "set_paths", "get_paths");

	ClassDB::bind_method(D_METHOD("set_point_path", "point_path"), &IntersectionNode::set_point_path);
	ClassDB::bind_method(D_METHOD("get_point_path"), &IntersectionNode::get_point_path);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "point_path"), "set_point_path", "get_point_path");

	ClassDB::bind_method(D_METHOD("set_curves", "curves"), &IntersectionNode::set_curves);
	ClassDB::bind_method(D_METHOD("get_curves"), &IntersectionNode::get_curves);
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "curves"), "set_curves", "get_curves");

	ClassDB::bind_method(D_METHOD("set_intersection_material", "material"), &IntersectionNode::set_intersection_material);
	ClassDB::bind_method(D_METHOD("get_intersection_material"), &IntersectionNode::get_intersection_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "intersection_material", PROPERTY_HINT_RESOURCE_TYPE, "StandardMaterial3D"), "set_intersection_material", "get_intersection_material");

	ClassDB::bind_method(D_METHOD("set_show_debug", "show_debug"), &IntersectionNode::set_show_debug);
	ClassDB::bind_method(D_METHOD("get_show_debug"), &IntersectionNode::get_show_debug);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug"), "set_show_debug", "get_show_debug");
}

void IntersectionNode::_ready() {
	last_transform = get_transform();

	intersection_array_mesh.instantiate();
	intersection_mesh = memnew(MeshInstance3D);
	intersection_mesh->set_material_override(intersection_material);
	intersection_mesh->set_mesh(intersection_array_mesh);
	add_child(intersection_mesh);

	on_build_junction();
	get_verts();
	build_mesh();
	refresh_debug_boxes();

	set_process(true);
}

void IntersectionNode::_process(double p_delta) {
	if (!get_transform().is_equal_approx(last_transform)) {
		on_build_junction();
		get_verts();
		build_mesh();
		refresh_debug_boxes();

		last_transform = get_transform();
	}
}

Vector3 IntersectionNode::evaluate_position(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, float t) const {
	Vector3 q0 = p0.lerp(p1, t);
	Vector3 q1 = p1.lerp(p2, t);
	return q0.lerp(q1, t);
}

int IntersectionNode::sort_points(const Vector3 &p_center, const Vector3 &point1, const Vector3 &point2) const {
	Vector3 direction1 = point1 - p_center;
	Vector3 direction2 = point2 - p_center;

	float angle1 = Math::atan2(direction1.z, direction1.x);
	float angle2 = Math::atan2(direction2.z, direction2.x);

	if (angle1 < 0)
		angle1 += Math_TAU;
	if (angle2 < 0)
		angle2 += Math_TAU;

	if (angle1 < angle2)
		return -1;
	if (angle1 > angle2)
		return 1;
	return 0;
}

void IntersectionNode::on_build_junction() {
	intersection.clear();
	int length = paths.size();

	for (int i = 0; i < length; i++) {
		NodePath np = paths[i];
		if (np.is_empty()) {
			continue;
		}

		Node *node = get_node_or_null(np);
		if (!node) {
			continue;
		}

		Path3D *path = Object::cast_to<Path3D>(node);
		if (!path) {
			continue;
		}

		int point = (i < point_path.size()) ? point_path[i] : 0;
		intersection.add_junction(path, point);
	}
}

void IntersectionNode::get_verts() {
	curve_points.clear();

	std::vector<JunctionEdge> junction_edges;
	center = Vector3();

	const Vector<JunctionInfo> &junctions = intersection.get_junctions();

	for (int i = 0; i < junctions.size(); i++) {
		const JunctionInfo &junction = junctions[i];

		if (!junction.path) {
			continue;
		}

		float t = (junction.point == 0) ? 0.0f : 1.0f;

		// this shit costs me 3 hours to solve, because i hardcoded the width value lmao
		// now it gets the width of the road node
		RoadNode *road_node = Object::cast_to<RoadNode>(junction.path);
		float road_width = road_node ? road_node->get_width() : 1.0f;

		SplineWidthResult sw = RoadTools::sample_spline_width(junction.path, road_width, t);

		Vector3 global_p1 = to_local(junction.path->to_global(sw.p1));
		Vector3 global_p2 = to_local(junction.path->to_global(sw.p2));

		if (junction.point == 0) {
			junction_edges.push_back(JunctionEdge(global_p1, global_p2));
		} else {
			junction_edges.push_back(JunctionEdge(global_p2, global_p1));
		}

		center += global_p1;
		center += global_p2;
	}

	int edge_count = (int)junction_edges.size();
	if (edge_count == 0) {
		return;
	}

	center /= (float)(edge_count * 2);

	std::sort(junction_edges.begin(), junction_edges.end(), [this](const JunctionEdge &x, const JunctionEdge &y) {
		return sort_points(center, x.center(), y.center()) < 0;
	});

	for (int j = 0; j < edge_count; j++) {
		Vector3 current_left = junction_edges[j].left;
		Vector3 current_right = junction_edges[j].right;

		int next_index = (j + 1) % edge_count;
		Vector3 next_left = junction_edges[next_index].left;

		curve_points.push_back(current_left);
		curve_points.push_back(current_right);

		Vector3 p0 = current_right;
		Vector3 p2 = next_left;

		float curve_factor = (j < curves.size()) ? (float)curves[j] : 0.5f;
		Vector3 p1 = p0.lerp(p2, 0.5f).lerp(center, curve_factor);

		int segments = 6;
		for (int i = 1; i < segments; i++) {
			float t = (float)i / segments;
			Vector3 pos = evaluate_position(p0, p1, p2, t);
			curve_points.push_back(pos);
		}
	}
}

void IntersectionNode::build_mesh() {
	if (!intersection_array_mesh.is_valid()) {
		return;
	}

	intersection_array_mesh->clear_surfaces();

	Array surface_array;
	surface_array.resize(Mesh::ARRAY_MAX);

	std::vector<Vector3> verts;
	std::vector<Vector3> normals;
	std::vector<int32_t> indices;

	int points_offset = (int)verts.size();

	int point_count = curve_points.size();
	for (int j = 1; j <= point_count; j++) {
		verts.push_back(center);
		verts.push_back(curve_points[j - 1]);

		if (j == point_count) {
			verts.push_back(curve_points[0]);
		} else {
			verts.push_back(curve_points[j]);
		}

		indices.push_back(points_offset + ((j - 1) * 3) + 0);
		indices.push_back(points_offset + ((j - 1) * 3) + 1);
		indices.push_back(points_offset + ((j - 1) * 3) + 2);

		Vector3 normal = Vector3(0, 1, 0);
		normals.push_back(normal);
		normals.push_back(normal);
		normals.push_back(normal);
	}

	PackedVector3Array packed_verts;
	PackedVector3Array packed_normals;
	PackedInt32Array packed_indices;

	packed_verts.resize((int)verts.size());
	packed_normals.resize((int)normals.size());
	packed_indices.resize((int)indices.size());

	Vector3 *verts_w = packed_verts.ptrw();
	Vector3 *normals_w = packed_normals.ptrw();
	int32_t *indices_w = packed_indices.ptrw();

	for (int i = 0; i < (int)verts.size(); i++) {
		verts_w[i] = verts[i];
		normals_w[i] = normals[i];
	}
	for (int i = 0; i < (int)indices.size(); i++) {
		indices_w[i] = indices[i];
	}

	surface_array[Mesh::ARRAY_VERTEX] = packed_verts;
	surface_array[Mesh::ARRAY_INDEX] = packed_indices;
	surface_array[Mesh::ARRAY_NORMAL] = packed_normals;

	intersection_array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface_array);
}

void IntersectionNode::clear_debug_boxes() {
	for (int i = 0; i < debug_instances.size(); i++) {
		if (debug_instances[i]) {
			debug_instances[i]->queue_free();
		}
	}
	debug_instances.clear();
}

void IntersectionNode::debug_boxes() {
	clear_debug_boxes();

	if (!show_debug) {
		return;
	}

	Ref<StandardMaterial3D> debug_mat;
	debug_mat.instantiate();
	debug_mat->set_albedo(Color(0.4f, 0.8f, 1.0f));
	debug_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	for (int i = 0; i < curve_points.size(); i++) {
		MeshInstance3D *mi = memnew(MeshInstance3D);

		Ref<BoxMesh> box_mesh;
		box_mesh.instantiate();
		box_mesh->set_size(Vector3(0.1f, 0.1f, 0.1f));

		mi->set_mesh(box_mesh);
		mi->set_position(curve_points[i]);
		mi->set_material_override(debug_mat);
		add_child(mi);
		debug_instances.push_back(mi);
	}
}

void IntersectionNode::refresh_debug_boxes() {
	if (show_debug) {
		debug_boxes();
	} else {
		clear_debug_boxes();
	}
}

void IntersectionNode::set_show_debug(bool p_show_debug) {
	show_debug = p_show_debug;
	if (is_inside_tree()) {
		refresh_debug_boxes();
	}
}

bool IntersectionNode::get_show_debug() const {
	return show_debug;
}

void IntersectionNode::set_paths(const TypedArray<NodePath> &p_paths) {
	paths = p_paths;
	if (!intersection_array_mesh.is_valid()) {
		return;
	}
	on_build_junction();
	get_verts();
	build_mesh();
	refresh_debug_boxes();
}

TypedArray<NodePath> IntersectionNode::get_paths() const {
	return paths;
}

void IntersectionNode::set_point_path(const PackedInt32Array &p_point_path) {
	point_path = p_point_path;
	if (!intersection_array_mesh.is_valid()) {
		return;
	}
	on_build_junction();
	get_verts();
	build_mesh();
	refresh_debug_boxes();
}

PackedInt32Array IntersectionNode::get_point_path() const {
	return point_path;
}

void IntersectionNode::set_curves(const PackedFloat32Array &p_curves) {
	curves = p_curves;
	if (!intersection_array_mesh.is_valid()) {
		return;
	}
	on_build_junction();
	get_verts();
	build_mesh();
	refresh_debug_boxes();
}

PackedFloat32Array IntersectionNode::get_curves() const {
	return curves;
}

void IntersectionNode::set_intersection_material(const Ref<StandardMaterial3D> &p_material) {
	intersection_material = p_material;
	if (intersection_mesh) {
		intersection_mesh->set_material_override(intersection_material);
	}
}

Ref<StandardMaterial3D> IntersectionNode::get_intersection_material() const {
	return intersection_material;
}

} // namespace godot