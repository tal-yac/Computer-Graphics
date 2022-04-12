#pragma once

#include "igl/opengl/glfw/renderer.h"
#include "Assignment1.h"
#include "InputManager.h"


namespace Window_config {
	static constexpr int DISPLAY_WIDTH = 900;
	static constexpr int DISPLAY_HEIGHT = 800;
	static constexpr float CAMERA_ANGLE = 0.0f;
	static constexpr float NEAR = 1.0f;
	static constexpr float FAR = 120.0f;
	//static constexpr int INFO_INDEX = 2;
}

int main(int argc, char *argv[])
{
	std::list<int> x, y;
	//x.push_back(DISPLAY_WIDTH / 2);
	x.push_back(Window_config::DISPLAY_WIDTH);
	y.push_back(Window_config::DISPLAY_HEIGHT);
	Display disp = Display(Window_config::DISPLAY_WIDTH, Window_config::DISPLAY_HEIGHT, "OPENGL");
	igl::opengl::glfw::imgui::ImGuiMenu *menu = nullptr;
	Renderer rndr(Window_config::CAMERA_ANGLE, (float)Window_config::DISPLAY_WIDTH / (float)Window_config::DISPLAY_HEIGHT, Window_config::NEAR, Window_config::FAR);
	Assignment1 scn; // initializing scene

	Init(disp, menu);				// adding callback functions
	scn.Init();					// adding shaders, textures, shapes to scene
	rndr.Init(&scn, x, y, 1, menu); // adding scene and viewports to the renderer
	disp.SetRenderer(&rndr);

	disp.launch_rendering(&rndr);

	return 0;
}
