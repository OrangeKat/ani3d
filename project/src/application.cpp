#include "application.hpp"

#include "environment.hpp"
#include "fire_scene.hpp"
#include "cgp/cgp.hpp"
#include <iostream>

using namespace cgp;

namespace
{
	fire_scene scene;
	timer_fps fps_record;

	window_structure standard_window_initialization();
	void initialize_default_shaders();
	void animation_loop();
	void display_gui_default();
	void window_size_callback(GLFWwindow* window, int width, int height);
	void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
	void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
	void keyboard_callback(GLFWwindow* window, int key, int, int action, int mods);
}

int run_application(int, char* argv[])
{
	std::cout << "Starting fire particle scene" << std::endl;

	initialize_project_settings(argv[0]);
	scene.window = standard_window_initialization();
	initialize_default_shaders();

	scene.initialize();

	std::cout << "Initialization done, entering animation loop" << std::endl;

	fps_record.start();

	while (!glfwWindowShouldClose(scene.window.glfw_window)) {
		animation_loop();

		if (project_settings::fps_limiting) {
			static double last_time = glfwGetTime();
			while (glfwGetTime() < last_time + 1.0 / project_settings::fps_max) {}
			last_time = glfwGetTime();
		}
	}

	cgp::imgui_cleanup();
	glfwDestroyWindow(scene.window.glfw_window);
	glfwTerminate();
	return 0;
}

namespace
{

void animation_loop()
{
	scene.camera_projection.aspect_ratio = scene.window.aspect_ratio();
	scene.environment.camera_projection = scene.camera_projection.matrix();

	glViewport(0, 0, scene.window.width, scene.window.height);

	vec3 const& background_color = scene.environment.background_color;
	glClearColor(background_color.x, background_color.y, background_color.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	float const time_interval = fps_record.update();
	if (fps_record.event) {
		std::string const title = "CGP Fire - " + str(fps_record.fps) + " fps";
		glfwSetWindowTitle(scene.window.glfw_window, title.c_str());
	}

	imgui_create_frame();
	ImGui::GetIO().FontGlobalScale = project_settings::gui_scale;
	ImGui::Begin("GUI", NULL, ImGuiWindowFlags_AlwaysAutoResize);
	scene.inputs.mouse.on_gui = ImGui::GetIO().WantCaptureMouse;
	scene.inputs.time_interval = time_interval;

	display_gui_default();
	scene.display_gui();

	scene.idle_frame();
	scene.display_frame();

	ImGui::End();
	imgui_render_frame(scene.window.glfw_window);
	glfwSwapBuffers(scene.window.glfw_window);
	glfwPollEvents();
}

window_structure standard_window_initialization()
{
	scene.window.initialize_glfw();

	int window_width = int(project_settings::initial_window_size_width);
	int window_height = int(project_settings::initial_window_size_height);
	if (project_settings::initial_window_size_width < 1)
		window_width = int(project_settings::initial_window_size_width * scene.window.monitor_width());
	if (project_settings::initial_window_size_height < 1)
		window_height = int(project_settings::initial_window_size_height * scene.window.monitor_height());

	window_structure window;
	window.create_window(window_width, window_height, "CGP Fire", CGP_OPENGL_VERSION_MAJOR, CGP_OPENGL_VERSION_MINOR);

	cgp::imgui_init(window.glfw_window);

	glfwSetMouseButtonCallback(window.glfw_window, mouse_click_callback);
	glfwSetCursorPosCallback(window.glfw_window, mouse_move_callback);
	glfwSetWindowSizeCallback(window.glfw_window, window_size_callback);
	glfwSetKeyCallback(window.glfw_window, keyboard_callback);
	glfwSetScrollCallback(window.glfw_window, mouse_scroll_callback);

	return window;
}

void initialize_default_shaders()
{
	std::string default_path_shaders = project_settings::path + "shaders/";

	mesh_drawable::default_shader.load(default_path_shaders + "mesh/mesh.vert.glsl",
		default_path_shaders + "mesh/mesh.frag.glsl");
	triangles_drawable::default_shader.load(default_path_shaders + "mesh/mesh.vert.glsl",
		default_path_shaders + "mesh/mesh.frag.glsl");

	image_structure const white_image = image_structure{ 1,1,image_color_type::rgba,{255,255,255,255} };
	mesh_drawable::default_texture.initialize_texture_2d_on_gpu(white_image);
	triangles_drawable::default_texture.initialize_texture_2d_on_gpu(white_image);

	curve_drawable::default_shader.load(default_path_shaders + "single_color/single_color.vert.glsl",
		default_path_shaders + "single_color/single_color.frag.glsl");
}

void display_gui_default()
{
	std::string fps_txt = str(fps_record.fps) + " fps";
	if (scene.inputs.keyboard.ctrl)
		fps_txt += " [ctrl]";
	if (scene.inputs.keyboard.shift)
		fps_txt += " [shift]";
	ImGui::Text("%s", fps_txt.c_str());

	if (ImGui::CollapsingHeader("Window")) {
		ImGui::Indent();
#ifndef __EMSCRIPTEN__
		bool changed_screen_mode = ImGui::Checkbox("Full Screen", &scene.window.is_full_screen);
		if (changed_screen_mode) {
			if (scene.window.is_full_screen)
				scene.window.set_full_screen();
			else
				scene.window.set_windowed_screen();
		}
#endif
		ImGui::SliderFloat("Gui Scale", &project_settings::gui_scale, 0.5f, 2.5f);
#ifndef __EMSCRIPTEN__
		ImGui::Checkbox("FPS limiting", &project_settings::fps_limiting);
		if (project_settings::fps_limiting) {
			ImGui::SliderFloat("FPS limit", &project_settings::fps_max, 10.0f, 250.0f);
		}
#endif
		if (ImGui::Checkbox("vsync", &project_settings::vsync)) {
			project_settings::vsync ? glfwSwapInterval(1) : glfwSwapInterval(0);
		}

		std::string window_size = "Window " + str(scene.window.width) + " x " + str(scene.window.height);
		ImGui::Text("%s", window_size.c_str());
		ImGui::Unindent();
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
	}
}

void window_size_callback(GLFWwindow*, int width, int height)
{
	scene.window.width = width;
	scene.window.height = height;
}

void mouse_move_callback(GLFWwindow*, double xpos, double ypos)
{
	vec2 const pos_relative = scene.window.convert_pixel_to_relative_coordinates({ float(xpos), float(ypos) });
	scene.inputs.mouse.position.update(pos_relative);
	scene.mouse_move_event();
}

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	scene.inputs.mouse.click.update_from_glfw_click(button, action);
	scene.mouse_click_event();
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	scene.inputs.mouse.scroll = float(yoffset);
	scene.mouse_scroll_event();
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	bool imgui_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;
	if (!imgui_capture_keyboard) {
		scene.inputs.keyboard.update_from_glfw_key(key, action);
		scene.keyboard_event();
	}
}

} // namespace

