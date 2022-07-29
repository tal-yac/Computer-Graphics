#pragma once

#include "GuiState.h"
#include "Project.h"
#include "igl/opengl/glfw/imgui/ImGuiMenu.h"
#include "imgui.h"
#include <igl/project.h>

class Icon {
public:
  Icon(const char *filename);
  ~Icon();
  GLuint *getTexture();
  int getWidth();
  int getHeight();

private:
  GLuint *texture;
  int width;
  int height;
};

class Gui : public igl::opengl::glfw::imgui::ImGuiMenu {
public:
  Gui();
  IGL_INLINE virtual void init(Display *disp);
  IGL_INLINE virtual void
  draw_viewer_menu(igl::opengl::glfw::Viewer *viewer,
                   std::vector<igl::opengl::Camera *> &camera,
                   Eigen::Vector4i &viewWindow,
                   std::vector<DrawInfo *> drawInfos) override;
  ~Gui();

private:
  ImFont *font = nullptr;
  ImFont *boldFont = nullptr;
};