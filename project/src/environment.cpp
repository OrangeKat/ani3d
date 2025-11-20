#include "environment.hpp"

using namespace cgp;

void environment_structure::send_opengl_uniform(opengl_shader_structure const& shader, bool expected) const
{
	opengl_uniform(shader, "projection", camera_projection, expected);
	opengl_uniform(shader, "view", camera_view, expected);
	opengl_uniform(shader, "light", light, false);
	uniform_generic.send_opengl_uniform(shader, expected);
}

std::string project_settings::path = "";
float project_settings::gui_scale = 1.0f;

// Uncapped FPS settings
bool project_settings::fps_limiting = false; 
float project_settings::fps_max = 144.0f; 
bool project_settings::vsync = false;

float project_settings::initial_window_size_width = 0.6f;
float project_settings::initial_window_size_height = 0.6f;

void initialize_project_settings(const char* executable_path)
{
	project_settings::path = cgp::project_path_find(executable_path, "shaders/");
}