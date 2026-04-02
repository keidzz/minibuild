#include "road_node.h"
#include "road_tools.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

RoadNode::RoadNode() {
}

RoadNode::~RoadNode() {
}

void RoadNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_resolution", "resolution"), &RoadNode::set_resolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &RoadNode::get_resolution);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "resolution"), "set_resolution", "get_resolution");

	ClassDB::bind_method(D_METHOD("set_width", "width"), &RoadNode::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &RoadNode::get_width);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "width"), "set_width", "get_width");

	ClassDB::bind_method(D_METHOD("set_road_material", "material"), &RoadNode::set_road_material);
	ClassDB::bind_method(D_METHOD("get_road_material"), &RoadNode::get_road_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "road_material", PROPERTY_HINT_RESOURCE_TYPE, "StandardMaterial3D"), "set_road_material", "get_road_material");

	ClassDB::bind_method(D_METHOD("set_show_debug", "show_debug"), &RoadNode::set_show_debug);
	ClassDB::bind_method(D_METHOD("get_show_debug"), &RoadNode::get_show_debug);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug"), "set_show_debug", "get_show_debug");
}

void RoadNode::_ready() {
	road_array_mesh.instantiate();
	road_mesh = memnew(MeshInstance3D);
	road_mesh->set_material_override(road_material);
	road_mesh->set_mesh(road_array_mesh);
	add_child(road_mesh);

	RoadTools::get_verts(this, resolution, width, m_verts_p1, m_verts_p2);
	build_mesh();

	connect("curve_changed", callable_mp(this, &RoadNode::_on_curve_changed));
}

void RoadNode::_on_curve_changed() {
	RoadTools::get_verts(this, resolution, width, m_verts_p1, m_verts_p2);
	build_mesh();
}

void RoadNode::build_mesh() {
	road_array_mesh->clear_surfaces();

	int length = m_verts_p2.size();
	if (length < 2) {
		return;
	}

	int quad_count = length - 1;
	int vert_count = quad_count * 4;
	int index_count = quad_count * 6;

	PackedVector3Array verts;
	PackedVector3Array normals;
	PackedInt32Array indices;
	verts.resize(vert_count);
	normals.resize(vert_count);
	indices.resize(index_count);

	Vector3 *verts_w = verts.ptrw();
	Vector3 *normals_w = normals.ptrw();
	int32_t *indices_w = indices.ptrw();

	const Vector3 normal_up(0, 1, 0);

	for (int i = 0; i < quad_count; i++) {
		int vi = i * 4;

		verts_w[vi + 0] = m_verts_p1[i];
		verts_w[vi + 1] = m_verts_p2[i];
		verts_w[vi + 2] = m_verts_p1[i + 1];
		verts_w[vi + 3] = m_verts_p2[i + 1];

		normals_w[vi + 0] = normal_up;
		normals_w[vi + 1] = normal_up;
		normals_w[vi + 2] = normal_up;
		normals_w[vi + 3] = normal_up;

		int ii = i * 6;
		indices_w[ii + 0] = vi + 0;
		indices_w[ii + 1] = vi + 2;
		indices_w[ii + 2] = vi + 3;
		indices_w[ii + 3] = vi + 3;
		indices_w[ii + 4] = vi + 1;
		indices_w[ii + 5] = vi + 0;
	}

	Array surface_array;
	surface_array.resize(Mesh::ARRAY_MAX);
	surface_array[Mesh::ARRAY_VERTEX] = verts;
	surface_array[Mesh::ARRAY_NORMAL] = normals;
	surface_array[Mesh::ARRAY_INDEX] = indices;

	road_array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface_array);
}

void RoadNode::clear_debug_boxes() {
	for (int i = 0; i < debug_instances.size(); i++) {
		if (debug_instances[i]) {
			debug_instances[i]->queue_free();
		}
	}
	debug_instances.clear();
}

void RoadNode::debug_boxes() {
	clear_debug_boxes();

	if (!show_debug) {
		return;
	}

	auto add_box = [this](const Vector3 &p) {
		MeshInstance3D *mi = memnew(MeshInstance3D);

		Ref<BoxMesh> box_mesh;
		box_mesh.instantiate();
		box_mesh->set_size(Vector3(0.1f, 0.1f, 0.1f));

		mi->set_mesh(box_mesh);
		mi->set_position(p);

		add_child(mi);
		debug_instances.push_back(mi);
	};

	for (int i = 0; i < m_verts_p1.size(); i++) {
		add_box(m_verts_p1[i]);
	}

	for (int i = 0; i < m_verts_p2.size(); i++) {
		add_box(m_verts_p2[i]);
	}
}

void RoadNode::refresh_debug_boxes() {
	if (show_debug) {
		debug_boxes();
	} else {
		clear_debug_boxes();
	}
}

void RoadNode::set_show_debug(bool p_show_debug) {
	show_debug = p_show_debug;
	if (is_inside_tree()) {
		refresh_debug_boxes();
	}
}

bool RoadNode::get_show_debug() const {
	return show_debug;
}

void RoadNode::set_resolution(int p_resolution) {
	resolution = p_resolution;
}

int RoadNode::get_resolution() const {
	return resolution;
}

void RoadNode::set_width(float p_width) {
	width = p_width;
}

float RoadNode::get_width() const {
	return width;
}

void RoadNode::set_road_material(const Ref<StandardMaterial3D> &p_material) {
	road_material = p_material;
	if (road_mesh) {
		road_mesh->set_material_override(road_material);
	}
}

Ref<StandardMaterial3D> RoadNode::get_road_material() const {
	return road_material;
}

} // namespace godot
