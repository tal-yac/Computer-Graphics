#include "Assignment3.h"
#include "external\glad\include\glad\glad.h"
#include <iostream>

#define STRINGIFY(X) STRINGIFY2(X)
#define STRINGIFY2(X) #X

static constexpr int CUBE_SIZE = 3;
static constexpr int TOTAL_FRAMES = 100;
// 90 degrees in radians
static constexpr float ROTATION_DEGREE = -3.141592653589793116 / 2.0f;
static constexpr float ROTATION_AMOUNT = ROTATION_DEGREE / TOTAL_FRAMES;

static void printMat(const Eigen::Matrix4d &mat) {
  std::cout << " matrix:" << std::endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      std::cout << mat(j, i) << " ";
    std::cout << std::endl;
  }
}

template <typename T> void print_vec(std::string &&name, std::vector<T> &v) {
  std::cout << name << ": ";
  for (auto i = v.begin(); i != v.end(); i++)
    std::cout << *i << " ";
  std::cout << std::endl;
}

void printFace(int cubeSize, std::vector<int> &Cube) {
  for (int i = 0; i < cubeSize; i++) {
    for (int j = 0; j < cubeSize; j++)
      std::cout << Cube[(cubeSize - i - 1) * cubeSize + j] << " ";
    std::cout << std::endl;
  }
}

void Assignment3::print_cube() {
  std::cout << "CUBE" << '\n';
  for (int i = 0; i < cubes_indices.size(); i++) {
    if (i < 10)
      std::cout << "0";
    std::cout << i << " ";
  }
  std::cout << '\n';
  for (const auto &i : cubes_indices) {
    if (i < 10)
      std::cout << "0";
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

Assignment3::Assignment3() {
  anim_speed = 10;
  curr_frame = 0;
  rotate_clockwise = true;
}

// Assignment3::Assignment3(float angle ,float relationWH, float near, float
// far) : Scene(angle,relationWH,near,far)
//{
// }

void Assignment3::Init() {
  unsigned int texIDs[3] = {0, 1, 2};
  unsigned int slots[3] = {0, 1, 2};
  AddShader("shaders/pickingShader2");
  AddShader("shaders/basicShader2");
  AddMaterial(texIDs, slots, 1);
  AddMaterial(texIDs + 1, slots + 1, 1);
  add_cube_texture();
  update_animation_speed(0);
  pickedShape = -1;
}

void Assignment3::Update(const Eigen::Matrix4f &Proj,
                         const Eigen::Matrix4f &View,
                         const Eigen::Matrix4f &Model, unsigned int shaderIndx,
                         unsigned int shapeIndx) {
  Shader *s = shaders[shaderIndx];
  int r = ((shapeIndx + 1) & 0x000000FF) >> 0;
  int g = ((shapeIndx + 1) & 0x0000FF00) >> 8;
  int b = ((shapeIndx + 1) & 0x00FF0000) >> 16;
  if (data_list[shapeIndx]->GetMaterial() >= 0 && !materials.empty()) {
    BindMaterial(s, data_list[shapeIndx]->GetMaterial());
  }
  textures[0]->Bind(0);
  s->Bind();
  s->SetUniformMat4f("View", View);
  s->SetUniformMat4f("Model", Model);
  s->SetUniformMat4f("Proj", Proj);
  if (shaderIndx == 0)
    s->SetUniform4f("lightColor", r / 255.0, g / 255.0, b / 255.0, 0);
  s->Unbind();
}

void Assignment3::WhenRotate() {}

void Assignment3::WhenTranslate() {}

void Assignment3::Animate() {
  read_operation();
  if (isActive) {
  }
}

void Assignment3::ScaleAllShapes(float amt, int viewportIndx) {
  for (int i = 1; i < data_list.size(); i++) {
    if (data_list[i]->Is2Render(viewportIndx)) {
      data_list[i]->MyScale(Eigen::Vector3d(amt, amt, amt));
    }
  }
}

void Assignment3::toggle_rotation_dir() {
  rotate_clockwise = !rotate_clockwise;
}

void Assignment3::update_animation_speed(int change) {
  anim_speed += change;
  if (anim_speed < 1)
    anim_speed = 1;
  else if (anim_speed > TOTAL_FRAMES)
    anim_speed = TOTAL_FRAMES;
  std::cout << "animation speed: " << anim_speed << std::endl;
}

std::vector<int> get_face_indices(int cube_size, int direction,
                                  int face_index) {
  std::vector<int> indices;
  switch (direction) {
  case 0: // col
    for (int i = 0; i < cube_size * cube_size; i++) {
      indices.push_back(cube_size * i + face_index);
    }
    break;
  case 1: // row
    for (int j = cube_size - 1; j >= 0; j--) {
      for (int i = cube_size - 1; i >= 0; i--) {
        int index = i * cube_size * cube_size + face_index * cube_size;
        indices.push_back(index + j);
      }
    }
    break;
  case 2: // the 3rd option
    for (int j = cube_size - 1; j >= 0; j--) {
      for (int i = cube_size - 1; i >= 0; i--) {
        int index = face_index * cube_size * cube_size;
        indices.push_back(index + i * cube_size + j);
      }
    }
    break;
  default:
    break;
  }
  return indices;
}

std::vector<int> rotated_face_indices(int cube_size, bool clockwise) {
  std::vector<int> new_positions;
  if (clockwise) {
    for (int i = 0; i < cube_size; i++) {        // col
      for (int j = cube_size - 1; j >= 0; j--) { // row
        new_positions.push_back(j * cube_size + i);
      }
    }
  } else {
    for (int j = cube_size - 1; j >= 0; j--) { // row
      for (int i = 0; i < cube_size; i++) {    // col
        new_positions.push_back(i * cube_size + j);
      }
    }
  }
  return new_positions;
}

void Assignment3::add_rotation(int face_direction, int face_index) {
  std::vector<int> old_positions =
      get_face_indices(CUBE_SIZE, face_direction, face_index);
  std::vector<int> rotated_positions = rotated_face_indices(
      CUBE_SIZE, face_direction == 2 ? rotate_clockwise : !rotate_clockwise);
  std::vector<int> values;
  for (int i = 0; i < CUBE_SIZE * CUBE_SIZE; i++) {
    values.push_back(cubes_indices[old_positions[i]]);
  }
  for (int i = 0; i < CUBE_SIZE * CUBE_SIZE; i++) {
    cubes_indices[old_positions[i]] = values[rotated_positions[i]];
  }
  Operation operation;
  operation.type = face_direction;
  operation.indices = values;
  print_cube();
  operations.push(operation);
}

void Assignment3::add_operation(int opcode) {
  // rotation
  if (opcode < 0)
    return;
  if (opcode < 6) {
    add_rotation(opcode / 2, (opcode % 2) * (CUBE_SIZE - 1));
    return;
  }
  Operation op;
  op.type = opcode - 3;
  operations.push(op);
}

void Assignment3::read_operation() {
  if (operations.empty())
    return;
  Operation op = operations.front();
  if (op.type < 3)
    return rotate_wall(op.type, op.indices);
  operations.pop();
  if (op.type == 3)
    return toggle_rotation_dir();
  update_animation_speed((op.type == 5) ? 1 : -1);
}

void Assignment3::rotate_wall(int type, std::vector<int> &indices) {
  curr_frame += anim_speed;
  float amount = ROTATION_AMOUNT;
  if (!rotate_clockwise)
    amount *= -1;
  if (curr_frame >= TOTAL_FRAMES) {
    amount *= (TOTAL_FRAMES + anim_speed - curr_frame);
    operations.pop();
    curr_frame = 0;
  } else
    amount *= anim_speed;
  int rotate_offset = xRotate;
  for (int i = 0; i < CUBE_SIZE * CUBE_SIZE; i++) {
    pickedShape = indices[i];
    ShapeTransformation(rotate_offset + type, amount, 1);
  }
  pickedShape = -1;
}

void Assignment3::WhenPick() {
  auto itr = std::find(cubes_indices.begin(), cubes_indices.end(), pickedShape);
  if (itr == cubes_indices.cend()) {
    std::cout << "Element not found" << std::endl;
    pickedShape = -1;
    return;
  }
  int cubeIndex = std::distance(cubes_indices.begin(), itr);
  std::cout << pickedShape << " present at index " << cubeIndex << std::endl;
  int faceIndex = cubeIndex / (CUBE_SIZE * CUBE_SIZE);
  cubeIndex -= faceIndex * CUBE_SIZE * CUBE_SIZE;
  int rowIndex = cubeIndex / CUBE_SIZE;
  int colIndex = cubeIndex - rowIndex * CUBE_SIZE;

  switch (pickedShapeNormalMax) {
  case 0:
    add_rotation(0, colIndex);
    break;
  case 1:
    add_rotation(1, rowIndex);
    break;
  case 2:
    add_rotation(2, faceIndex);
  default:
    break;
  }
  pickedShape = -1;
}

void Assignment3::add_cube_texture() {
  AddTexture("textures/plane.png", 2);
  AddTexture("textures/plane1.png", 2);
  float center = (CUBE_SIZE - 1.0f) / 2.0f;
  int pad = 1;
  for (int i = 0; i < CUBE_SIZE; i++) {
    for (int j = 0; j < CUBE_SIZE; j++) {
      for (int k = 0; k < CUBE_SIZE; k++) {
        AddShape(Cube, -1, TRIANGLES);
        pickedShape = i * CUBE_SIZE * CUBE_SIZE + j * CUBE_SIZE + k;
        SetShapeShader(pickedShape, 1);
        SetShapeMaterial(pickedShape, 0);
        ShapeTransformation(xTranslate, pad * (k - center), 0);
        ShapeTransformation(yTranslate, pad * (j - center), 0);
        ShapeTransformation(zTranslate, pad * (i - center), 0);
        cubes_indices.push_back(pickedShape);
      }
    }
  }
}

Assignment3::~Assignment3(void) {}