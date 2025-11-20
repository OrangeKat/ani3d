#include "fire_scene.hpp"
#include <algorithm>

using namespace cgp;

void fire_scene::initialize()
{
	camera_control.initialize(inputs, window);
	camera_control.set_rotation_axis_y();
	camera_control.look_at({ 3.0f, 1.6f, 3.0f }, { 0, 0.4f, 0 });
	environment.camera_view = camera_control.camera_model.matrix_view();
	environment.camera_projection = camera_projection.matrix();

	mesh sphere = mesh_primitive_sphere(0.5f);
	sphere.fill_empty_field();
	fire_particle.initialize_data_on_gpu(sphere);

	fire_particle.material.phong.ambient = 1.0f;
	fire_particle.material.phong.diffuse = 0.0f;
	fire_particle.material.phong.specular = 0.0f;
	fire_particle.material.texture_settings.active = false;

	mesh const campfire_mesh = mesh_load_file_obj(project_settings::path + "objects/campfire.obj");
	campfire.initialize_data_on_gpu(campfire_mesh);
	campfire.model.scaling = 1.0f;
	campfire.model.translation = { 0.0f, 0.0f, 0.0f };

	particles.reserve(max_particles);
	timer.start();
}

void fire_scene::emit_particles(float dt)
{
	if (!gui.emit) return;

	time_accumulator += dt;
	float particles_to_emit = emission_rate * time_accumulator;
	int count = static_cast<int>(particles_to_emit);
	if (count <= 0) return;

	time_accumulator -= count / emission_rate;

	for (int i = 0; i < count; ++i) {
		if (particles.size() >= max_particles) break;

		particle p;
		float r = std::sqrt(rand_uniform(0.0f, 1.0f)) * 0.6f;
		float theta = rand_uniform(0.0f, 2.0f * 3.14159f);
		p.position = { r * std::cos(theta), 0.0f, r * std::sin(theta) };

        p.velocity = { 
			rand_uniform(-0.1f, 0.1f), 
			0.1f + rand_uniform(0.0f, 0.2f),
			rand_uniform(-0.1f, 0.1f) 
		};

		p.life = p.max_life = particle_lifetime * rand_uniform(0.8f, 1.2f);
		p.size = particle_size * rand_uniform(0.8f, 1.2f);
		p.rotation_offset = rand_uniform(0.0f, 2.0f * 3.14159f);

		particles.push_back(p);
	}
}

void fire_scene::update_particles(float dt)
{
	float t = timer.t;

	for (auto& p : particles) {
		p.velocity.y += 2.0f * dt; 

		float centering_strength = 4.0f; 
		p.velocity.x -= p.position.x * centering_strength * dt; 
		p.velocity.z -= p.position.z * centering_strength * dt; 

		p.velocity.x *= 0.95f;
		p.velocity.z *= 0.95f;
			
		float turbulence = 2.0f;
		float n_x = noise_perlin({ p.position.x * 1.5f, p.position.y * 2.0f - t * 2.0f, t });
		float n_z = noise_perlin({ p.position.z * 1.5f, p.position.y * 2.0f - t * 2.0f, t + 10.0f });
		p.velocity.x += n_x * turbulence * dt;
		p.velocity.z += n_z * turbulence * dt;

		p.position += p.velocity * dt;
		p.life -= dt;
    }

	particles.erase(std::remove_if(particles.begin(), particles.end(),
		[](particle const& p) { return p.life <= 0.0f; }),
		particles.end());
}

void fire_scene::draw_particles()
{
	draw(campfire, environment);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);

	for (auto const& p : particles) {
		float life_ratio = p.life / p.max_life;
		float t = 1.0f - life_ratio; 

		vec3 color;
		if (gui.realistic_fire) {
			vec3 color_core = { 1.0f, 0.9f, 0.6f };  
			vec3 color_mid = { 1.0f, 0.5f, 0.05f };  
			vec3 color_edge = { 0.8f, 0.1f, 0.05f }; 
			vec3 color_smoke = { 0.05f, 0.05f, 0.05f };

			if (t < 0.2f)
				color = (1.0f - t / 0.2f) * color_core + (t / 0.2f) * color_mid;
			else if (t < 0.6f) {
				float local_t = (t - 0.2f) / 0.4f;
				color = (1.0f - local_t) * color_mid + local_t * color_edge;
			}
			else {
				float local_t = (t - 0.6f) / 0.4f;
				color = (1.0f - local_t) * color_edge + local_t * color_smoke;
			}

			float alpha_fade = 1.0f;
			if (life_ratio < 0.3f) alpha_fade = life_ratio / 0.3f;
			if (life_ratio > 0.9f) alpha_fade = (1.0f - life_ratio) / 0.1f;
			
			color *= alpha_fade * 0.8f;
		}
		else {
			if (life_ratio > 0.7f) color = { 1.6f, 1.2f, 0.6f };
			else if (life_ratio > 0.4f) color = { 1.6f, 0.8f, 0.2f };
			else color = { 1.2f, 0.3f, 0.05f };
			
			float noise_alpha = 0.5f + 0.5f * noise_perlin({ p.position.x, p.position.y, timer.t });
			color *= life_ratio * noise_alpha;
		}

		float size_scale = gui.realistic_fire ? (0.5f + 1.5f * t) : (0.4f + 0.6f * life_ratio);
		float current_size = p.size * size_scale;

		fire_particle.material.color = color;
		fire_particle.model.rotation = rotation_transform::from_axis_angle({0,0,1}, p.rotation_offset);
		fire_particle.model.translation = p.position;
		fire_particle.model.scaling_xyz = { current_size, current_size, current_size };

		draw(fire_particle, environment);
	}

	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}

void fire_scene::display_frame()
{
	float dt = timer.update();
	dt = clamp(dt, 0.0f, 0.05f); 

	emit_particles(dt);
	update_particles(dt);

	environment.light = camera_control.camera_model.position();
	draw_particles();
}

void fire_scene::display_gui()
{
	ImGui::Checkbox("Emit Particles", &gui.emit);
	ImGui::SliderFloat("Particle Size", &particle_size, 0.02f, 0.15f);
	ImGui::SliderFloat("Emission Rate", &emission_rate, 100.0f, 2000.0f);
	ImGui::SliderFloat("Lifetime", &particle_lifetime, 0.3f, 2.0f);
}

void fire_scene::mouse_move_event()
{
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move(environment.camera_view);
}

void fire_scene::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}
void fire_scene::keyboard_event()
{
	camera_control.action_keyboard(environment.camera_view);
}
void fire_scene::idle_frame()
{
	camera_control.idle_frame(environment.camera_view);
}