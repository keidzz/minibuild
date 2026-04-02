#pragma once

#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/path3d.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {

struct SplineEvalResult {
	Vector3 position;
	Vector3 tangent;
	Vector3 up;
	bool valid;
};

struct SplineWidthResult {
	Vector3 p1;
	Vector3 p2;
};

class RoadTools {
public:
	static SplineEvalResult evaluate(Path3D *p_path, float p_t);
	static SplineWidthResult sample_spline_width(Path3D *p_path, float p_width, float p_t);
	static void get_verts(Path3D *p_path, int p_resolution, float p_width,
			Vector<Vector3> &r_verts_p1, Vector<Vector3> &r_verts_p2);
};

} //namespace godot

