#include "fire_scene.hpp"
#include <algorithm>

using namespace cgp;

void fire_scene::initialize()
{
    camera_control.initialize(inputs, window);
    camera_control.set_rotation_axis_y();
    camera_control.look_at({3.0f,1.6f,3.0f}, {0,0.4f,0});
    environment.camera_view = camera_control.camera_model.matrix_view();
    environment.camera_projection = camera_projection.matrix();

    mesh sphere = mesh_primitive_sphere(0.5f);
    sphere.fill_empty_field();
    fire_particle.initialize_data_on_gpu(sphere);

    fire_particle.material.color = {1.5f,0.6f,0.2f};
    fire_particle.material.phong.ambient = 0.4f;
    fire_particle.material.phong.diffuse = 0.8f;
    fire_particle.material.phong.specular = 0.15f;
    fire_particle.material.phong.specular_exponent = 8.0f;
    fire_particle.material.texture_settings.active = false;
    fire_particle.material.texture_settings.inverse_v = false;

    particles.reserve(max_particles);
    timer.start();
}

void fire_scene::emit_particles(float dt)
{
    if(!gui.emit) return;

    time_accumulator += dt;
    float particles_to_emit = emission_rate * time_accumulator;
    int count = static_cast<int>(particles_to_emit);
    if(count <= 0) return;

    time_accumulator -= count / emission_rate;

    for(int i=0; i<count; ++i){
        if(particles.size() >= max_particles) break;

        particle p;
        p.position = { rand_uniform(-0.2f,0.2f), 0.0f, rand_uniform(-0.2f,0.2f) };
        float speed = 1.2f + rand_uniform(0.0f,0.6f);
        p.velocity = { rand_uniform(-0.15f,0.15f), speed, rand_uniform(-0.15f,0.15f) };
        p.life = p.max_life = particle_lifetime * rand_uniform(0.8f,1.2f);
        p.size = particle_size * rand_uniform(0.8f,1.2f);

        particles.push_back(p);
    }
}

void fire_scene::update_particles(float dt)
{
    float t = timer.t;

    for(auto& p : particles){
        p.velocity.y += 1.0f * dt;

        float n_x = noise_perlin({p.position.x*2.0f, p.position.y*2.0f, t});
        float n_z = noise_perlin({p.position.z*2.0f, p.position.y*2.0f, t});
        p.velocity.x += n_x * 0.5f * dt;
        p.velocity.z += n_z * 0.5f * dt;

        p.position += p.velocity * dt;
        p.life -= dt;
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](particle const& p){ return p.life <= 0.0f; }),
        particles.end());
}

void fire_scene::draw_particles()
{
    float t = timer.t;

    for(auto const& p : particles){
        float life_ratio = clamp(p.life / p.max_life, 0.0f, 1.0f);

        vec3 color;
        if(life_ratio > 0.7f) color = {1.6f,1.2f,0.6f};
        else if(life_ratio > 0.4f) color = {1.6f,0.8f,0.2f};
        else color = {1.2f,0.3f,0.05f};

        float noise_alpha = noise_perlin({p.position.x*3.0f, p.position.y*3.0f, t});
        noise_alpha = 0.5f + 0.5f * noise_alpha;
        color *= life_ratio * noise_alpha;

        float size = p.size * (0.4f + 0.6f * life_ratio);

        fire_particle.material.color = color;
        fire_particle.model.rotation = rotation_transform();
        fire_particle.model.translation = p.position;
        fire_particle.model.scaling_xyz = {size,size,size};

        draw(fire_particle, environment);
    }
}

void fire_scene::display_frame()
{
    float dt = timer.update();
    dt = clamp(dt,0.0f,0.05f);

    emit_particles(dt);
    update_particles(dt);

    environment.light = camera_control.camera_model.position();
    draw_particles();
}

void fire_scene::display_gui()
{
    ImGui::Checkbox("Emit Particles",&gui.emit);
    ImGui::Checkbox("Display",&gui.display_billboards);
    ImGui::Checkbox("Realistic Fire",&gui.realistic_fire);
    ImGui::SliderFloat("Particle Size",&particle_size,0.02f,0.4f);
    ImGui::SliderFloat("Emission Rate",&emission_rate,50.0f,500.0f);
    ImGui::SliderFloat("Lifetime",&particle_lifetime,0.3f,4.0f);
}

void fire_scene::mouse_move_event()
{
    if(!inputs.keyboard.shift)
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
