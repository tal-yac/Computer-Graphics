#pragma once

#include "Project.h"
#include "igl/opengl/glfw/Display.h"
#include "igl/opengl/glfw/renderer.h"
#include "imgui/imgui.h"

double xStart, yStart;

void glfw_mouse_callback(GLFWwindow *window, int button, int action, int mods) {
  Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
  Project *scn = (Project *)rndr->GetScene();
  if (scn->getAnimationStatus() == PLAYING)
    return;

  if (action == GLFW_PRESS) {
    Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
    Project *scn = (Project *)rndr->GetScene();
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      rndr->Pressed();
      glfwGetCursorPos(window, &xStart, &yStart);
    }
    rndr->UpdatePress(xStart, yStart);

  } else {
    Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
    Project *scn = (Project *)rndr->GetScene();
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      double xEnd, yEnd;
      glfwGetCursorPos(window, &xEnd, &yEnd);
      rndr->Pressed();
    }
  }
}

void glfw_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
  Project *scn = (Project *)rndr->GetScene();
  WindowLocation windowLoc = scn->GetWindowLocation();
  auto mouse = scn->GetMouse();
  float xpos = mouse[0];
  float ypos = mouse[1];
  if ((xpos >= windowLoc.topLeft.x && xpos <= windowLoc.bottomRight.x &&
       ypos >= windowLoc.topLeft.y && ypos <= windowLoc.bottomRight.y) ||
      scn->getAnimationStatus() == PLAYING) {
    return;
  }

  if (rndr->IsPicked()) {
    rndr->UpdateZpos((int)yoffset);
    rndr->MouseProccessing(GLFW_MOUSE_BUTTON_MIDDLE, 0,
                           scn->GetConrolledCameraId());
  } else {
    scn->MoveCamera(Renderer::zTranslate, -(float)yoffset);
  }
}

void glfw_cursor_position_callback(GLFWwindow *window, double xpos,
                                   double ypos) {
  Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
  Project *scn = (Project *)rndr->GetScene();
  WindowLocation windowLoc = scn->GetWindowLocation();
  scn->UpdateMouse((float)xpos, (float)ypos);
  if ((xpos >= windowLoc.topLeft.x && xpos <= windowLoc.bottomRight.x &&
       ypos >= windowLoc.topLeft.y && ypos <= windowLoc.bottomRight.y) ||
      scn->getAnimationStatus() == PLAYING)
    return;

  double xStart, yStart;
  rndr->UpdatePosition(
      (float)xpos,
      -(float)ypos);
  if (rndr->CheckViewport(xpos, ypos, scn->GetConrolledCameraId())) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      glfwGetCursorPos(window, &xStart, &yStart);
      if (!rndr->IsPressed()) {
        rndr->Pressed();
      }
      if (!scn->IsMousePressed())
        scn->RightClick(xStart, yStart);

    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ==
               GLFW_PRESS) {
      glfwGetCursorPos(window, &xStart, &yStart);
      scn->LeftClick(xStart, yStart);
      rndr->MouseProccessing(GLFW_MOUSE_BUTTON_LEFT, 0,
                             scn->GetConrolledCameraId());
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) ==
                   GLFW_RELEASE &&
               rndr->IsPressed()) {
      double xEnd, yEnd;
      glfwGetCursorPos(window, &xEnd, &yEnd);
      rndr->Pressed();
      scn->UnpressMouse(xEnd, yEnd);

    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ==
                   GLFW_RELEASE &&
               rndr->IsPressed()) {
      double xEnd, yEnd;
      glfwGetCursorPos(window, &xEnd, &yEnd);
      scn->UnpressMouse(xEnd, yEnd);
    }
  }
  auto &io = ImGui::GetIO();
  if (scn->IsMousePressed() && !io.MouseDown[0] && !io.MouseDown[1]) {
    double xEnd, yEnd;
    glfwGetCursorPos(window, &xEnd, &yEnd);
    scn->UnpressMouse(xEnd, yEnd);
  }
}

void glfw_window_size_callback(GLFWwindow *window, int width, int height) {
  Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
  Project *scn = (Project *)rndr->GetScene();
  rndr->resize(window, width, height);
  scn->UpdateResolution(width, height);
}

bool escPressed = false;
void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action,
                       int mods) {
  Renderer *rndr = (Renderer *)glfwGetWindowUserPointer(window);
  Project *scn = (Project *)rndr->GetScene();
  auto pickedShapes = scn->GetPickedShapes();
  std::vector<std::shared_ptr<ObjectMover>> pickedMovers;
  for (auto shp : pickedShapes) {
    if (std::find(pickedMovers.begin(), pickedMovers.end(), shp->mover) ==
        pickedMovers.end())
      pickedMovers.push_back(shp->mover);
  }
  if (scn->getAnimationStatus() == PLAYING)
    return;
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key != GLFW_KEY_ESCAPE)
      escPressed = false;
    switch (key) {
    case GLFW_KEY_ESCAPE:
      break;
    case GLFW_KEY_LEFT_CONTROL:
      scn->ChangeControlledCamera();
      break;
    case GLFW_KEY_UP:
      if (pickedShapes.empty())
        scn->MoveCamera(Renderer::xRotate, 0.05f);
      else
        for (auto mover : pickedMovers) {
          mover->shift(Eigen::Vector3f(0, 1, 0));
        }
      break;
    case GLFW_KEY_DOWN:
      if (pickedShapes.empty())
        scn->MoveCamera(Renderer::xRotate, -0.05f);
      else {
        for (auto mover : pickedMovers) {
          mover->shift(Eigen::Vector3f(0, -1, 0));
        }
      }
      break;
    case GLFW_KEY_LEFT:
      if (pickedShapes.empty())
        scn->MoveCamera(Renderer::yRotate, 0.05f);
      else {
        for (auto mover : pickedMovers) {
          mover->shift(Eigen::Vector3f(-1, 0, 0));
        }
      }
      break;
    case GLFW_KEY_RIGHT:
      if (pickedShapes.empty())
        scn->MoveCamera(Renderer::yRotate, -0.05f);
      else {
        for (auto mover : pickedMovers) {
          mover->shift(Eigen::Vector3f(1, 0, 0));
        }
      }
      break;
    case GLFW_KEY_KP_ADD:
    case GLFW_KEY_EQUAL:
      if (pickedShapes.empty()) {
      } else {
        for (auto mover : pickedMovers) {
          mover->shift(Eigen::Vector3f(0, 0, 1));
        }
      }
      break;
    case GLFW_KEY_KP_SUBTRACT:
    case GLFW_KEY_MINUS:
      if (pickedShapes.empty()) {
      } else {
        for (auto mover : pickedMovers) {
          mover->shift(Eigen::Vector3f(0, 0, -1));
        }
      }
      break;
    }
  }
}

void Init(Display &display, igl::opengl::glfw::imgui::ImGuiMenu *menu) {
  display.AddKeyCallBack(glfw_key_callback);
  display.AddMouseCallBacks(glfw_mouse_callback, glfw_scroll_callback,
                            glfw_cursor_position_callback);
  display.AddResizeCallBack(glfw_window_size_callback);
  menu->init(&display);
}
