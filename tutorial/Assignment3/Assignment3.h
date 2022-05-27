#pragma once
#include "igl/opengl/glfw/Viewer.h"

class Assignment3 : public igl::opengl::glfw::Viewer {

public:
  Assignment3();
  //	Assignment3(float angle,float relationWH,float near, float far);
  void Init();
  void Update(const Eigen::Matrix4f &Proj, const Eigen::Matrix4f &View,
              const Eigen::Matrix4f &Model, unsigned int shaderIndx,
              unsigned int shapeIndx);
  void WhenRotate();
  void WhenTranslate();
  void Animate() override;
  void ScaleAllShapes(float amt, int viewportIndx);
  void AddOperation(int operation);
  void ReadOperation();

  void update_animation_speed(int change);
  void toggle_rotation_dir();
  void addRotation(int faceDirection, int faceIndex);
  void rotateWall(int type, std::vector<int> indexs);
  void WhenPicked();

  void add_cube_texture();

  ~Assignment3(void);

private:
  int cube_size;
  int anim_speed;
  float curr_frame;
  bool rotate_clockwise;
  std::vector<int> cubesIndexs;
};
