#include "road_tools.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
SplineEvalResult RoadTools::evaluate(Path3D *p_path, float p_t) {
	SplineEvalResult result;
	result.valid = false;
	result.position = Vector3();
	result.tangent = Vector3();
	result.up = Vector3();

	if (!p_path) {
		return result;
	}

	Ref<Curve3D> curve = p_path->get_curve();
	if (curve.is_null() || curve->get_point_count() < 2) {
		return result;
	}

	p_t = CLAMP(p_t, 0.0f, 1.0f);

	float curve_length = curve->get_baked_length();
	float offset = p_t * curve_length;

	result.position = curve->sample_baked(offset);

	float delta_offset = MIN(0.01f, curve_length * 0.01f);

	if (offset + delta_offset <= curve_length) {
		Vector3 next_pos = curve->sample_baked(offset + delta_offset);
		result.tangent = (next_pos - result.position).normalized();
	} else if (offset - delta_offset >= 0.0f) {
		Vector3 prev_pos = curve->sample_baked(offset - delta_offset);
		result.tangent = (result.position - prev_pos).normalized();
	} else {
		result.tangent = Vector3(0, 0, -1);
	}

    result.up = Vector3(0, 1, 0);
    result.valid = true;
    
    return result;
}

SplineWidthResult RoadTools::sample_spline_width(Path3D *p_path, float p_width, float p_t) {
    SplineWidthResult result;

    SplineEvalResult eval = evaluate(p_path, p_t);
    Vector3 right = eval.tangent.cross(eval.up).normalized();

    result.p1 = eval.position - (right * p_width * 0.5f);
    result.p2 = eval.position + (right * p_width * 0.5f);

    return result;
}

void RoadTools::get_verts(Path3D *p_path, int p_resolution, float p_width, 
Vector<Vector3> &r_verts_p1, Vector<Vector3> &r_verts_p2) {
    r_verts_p1.clear();
    r_verts_p2.clear();

    for (int i = 0; i <= p_resolution; i++) {
        float t = (float)i / (float)p_resolution;
        SplineWidthResult sw = sample_spline_width(p_path, p_width, t);
        r_verts_p1.push_back(sw.p1);
        r_verts_p2.push_back(sw.p2);
    }
}


} //namespace godot