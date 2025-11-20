#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include <vector>

// Individual particle structure
struct particle {
	cgp::vec3 position;
	cgp::vec3 velocity;
	float life;
	float max_life;
	float size;
};

// GUI parameters
struct gui_parameters {
	bool emit = true;
	bool display_billboards = true;
	bool realistic_fire = true;
};

// Fire scene class
class fire_scene : public cgp::scene_inputs_generic
{
public:
	cgp::camera_controller_orbit_euler camera_control;
	cgp::camera_projection_perspective camera_projection;
	cgp::window_structure window;
	environment_structure environment;
	cgp::input_devices inputs;

	void initialize();
	void display_frame();
	void display_gui();
	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();

private:
	void emit_particles(float dt);
	void update_particles(float dt);
	void draw_particles();

	cgp::mesh_drawable fire_particle;
	std::vector<particle> particles;
	gui_parameters gui;
	cgp::timer_basic timer;
	size_t max_particles = 10000;
	float particle_size = 0.10f;
	float emission_rate = 100.0f;
	float particle_lifetime = 1.0f;
	float time_accumulator = 0.0f;
};
