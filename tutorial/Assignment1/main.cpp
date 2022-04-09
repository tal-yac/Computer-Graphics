#include "igl/opengl/glfw/renderer.h"
#include "Assignment1.h"
#include "InputManager.h"

static constexpr int DISPLAY_WIDTH = 1200;
static constexpr int DISPLAY_HEIGHT = 800;
static constexpr float CAMERA_ANGLE = 0.0f;
static constexpr float NEAR = 1.0f;
static constexpr float FAR = 120.0f;
static constexpr int INFO_INDEX = 2;

int main(int argc, char *argv[])
{
	std::list<int> x, y;
	//x.push_back(DISPLAY_WIDTH / 2);
	x.push_back(DISPLAY_WIDTH);
	y.push_back(DISPLAY_HEIGHT);
	Display disp = Display(DISPLAY_WIDTH, DISPLAY_HEIGHT, "OPENGL");
	igl::opengl::glfw::imgui::ImGuiMenu menu;
	Renderer rndr(CAMERA_ANGLE, (float)DISPLAY_WIDTH / (float)DISPLAY_HEIGHT / 2, NEAR, FAR);
	Assignment1 scn; // initializing scene

	Init(disp, &menu);				// adding callback functions
	scn.Init();					// adding shaders, textures, shapes to scene
	rndr.Init(&scn, x, y, 1, &menu); // adding scene and viewports to the renderer
	disp.SetRenderer(&rndr);

	disp.launch_rendering(&rndr);

	return 0;
}
