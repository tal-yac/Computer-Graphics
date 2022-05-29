#pragma once
#include "igl/opengl/glfw/Viewer.h"
#include <queue>

enum Cube_face { LEFT, RIGHT, DOWN, UP, BACK, FRONT };

struct Operation {
  int type;
  std::vector<int> indices;
};

struct Cube {
  unsigned int index;
};

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
  void add_operation(int opcode);
  void read_operation();

  void update_animation_speed(int change);
  void toggle_rotation_dir();
  void add_rotation(int face_direction, int face_index);
  void rotate_wall(int type, std::vector<int> &indices);
  void WhenPicked();

  void add_cube_texture();

  ~Assignment3(void);

private:
  int anim_speed;
  float curr_frame;
  bool rotate_clockwise;
  std::vector<int> cubes_indices;
  std::queue<Operation> operations;
};
