#pragma once

#include "ObjectMoverForGui.h"
#include "Project.h"


enum GuiStatus {
  MENU,
  ERROR,
  SHAPE_EDITING,
  SHADER_EDITING,
  CURVE_EDITING,
  MEDIA_SLIDER,
  CAMERA_ADD
};

enum GuiStep { CONTINUE, NEW, EXIT };

enum EntityEditingMode { CREATE_NEW, EDIT_EXISTING };

class GuiState {
public:
  GuiState(GuiStatus tag);
  GuiStatus getTag() { return tag; };
  virtual void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
                   Eigen::Vector4i &viewWindow,
                   std::vector<DrawInfo *> drawInfos, ImFont *font,
                   ImFont *boldFont) = 0;
  static void OpenErrorWindow(Project *project, std::string error);

private:
  GuiStatus tag;
};

class MenuState : public GuiState {
public:
  MenuState(float hidpi_scaling_, float pixel_ratio_);
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);
  float menu_scaling() { return hidpi_scaling_ / pixel_ratio_; }
  ~MenuState();

private:
  float hidpi_scaling_;
  float pixel_ratio_;
  char *newLayerName = strdup("");
  char *shadersSearchPattern = strdup("");
};

class ErrorMsgState : public GuiState {
public:
  ErrorMsgState(std::string errorMessage);
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);

private:
  std::string errorMessage;
};

class ShapeEditingState : public GuiState {
public:
  ShapeEditingState();
  ShapeEditingState(std::shared_ptr<SceneShape> shp,
                    std::shared_ptr<SceneShader> scnShader);
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);
  float sizePercents;
  ~ShapeEditingState();

private:
  EntityEditingMode editingMode;
  char *name;
  int shader;
  std::string texture;
  ShapeSource source;
  std::shared_ptr<igl::opengl::glfw::Viewer::shapes> type;
  std::string file;
  std::shared_ptr<ObjectMoverSplit> mover;
  std::shared_ptr<Layer> layer;
  Eigen::Vector3f lastDrawnPosition;
  int parent;
  std::vector<int> children;
};

class ShaderEditingState : public GuiState {
public:
  ShaderEditingState(std::shared_ptr<SceneShader> scnShader);
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);

private:
  std::vector<std::shared_ptr<ShaderParam>> shaderParams;
  std::string shaderName;
};

class MovementCurveEditingState : public GuiState {
public:
  MovementCurveEditingState(std::string name,
                            std::shared_ptr<ObjectMoverSplit> mover);
  MovementCurveEditingState();
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);
  float startTime;

private:
  std::vector<std::shared_ptr<ObjectMoverForGui>> movers;
  char *curveName;
  EntityEditingMode editingMode;
};

class MediaSliderState : public GuiState {
public:
  MediaSliderState();
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);
};

class CameraAddState : public GuiState {
public:
  CameraAddState();
  void Run(Project *project, std::vector<igl::opengl::Camera *> &camera,
           Eigen::Vector4i &viewWindow, std::vector<DrawInfo *> drawInfos,
           ImFont *font, ImFont *boldFont);

private:
  char *cameraName = strdup("");
  EntityEditingMode editingMode;
  std::shared_ptr<ObjectMoverSplit> mover;
  float *position;
};