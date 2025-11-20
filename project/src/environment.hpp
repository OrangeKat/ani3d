#pragma once

#include "cgp/cgp.hpp"

struct environment_structure : cgp::environment_generic_structure
{
	cgp::vec3 background_color = {0.02f, 0.02f, 0.035f};
	cgp::mat4 camera_view = cgp::mat4::build_identity();
	cgp::mat4 camera_projection = cgp::mat4::build_identity();
	cgp::vec3 light = {1.0f, 1.0f, 1.0f};
	cgp::uniform_generic_structure uniform_generic;

	void send_opengl_uniform(cgp::opengl_shader_structure const& shader, bool expected = default_expected_uniform) const override;
};

struct project_settings
{
	static std::string path;
	static float gui_scale;
	static bool fps_limiting;
	static float fps_max;
	static bool vsync;
	static float initial_window_size_width;
	static float initial_window_size_height;
};

void initialize_project_settings(const char* executable_path);

