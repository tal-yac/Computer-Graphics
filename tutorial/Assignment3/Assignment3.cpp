#include "Assignment3.h"
#include "external\glad\include\glad\glad.h"
#include <iostream>

static constexpr int CUBE_SIZE = 3;
static constexpr int TOTAL_FRAMES = 100;
// 90 degrees in radians
static constexpr float ROTATION_DEGREE = 1.5708;

static void printMat(const Eigen::Matrix4d &mat) {
  std::cout << " matrix:" << std::endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      std::cout << mat(j, i) << " ";
    std::cout << std::endl;
  }
}

void printFace(int cubeSize, std::vector<int> Cube) {
  for (int i = 0; i < cubeSize; i++) {
    for (int j = 0; j < cubeSize; j++)
      std::cout << Cube[(cubeSize - i - 1) * cubeSize + j] << " ";
    std::cout << std::endl;
  }
}

void printCube(int cubeSize, std::vector<int> Cube) {
  std::cout << "cube" << std::endl;
  for (int i = 0; i < cubeSize; i++) {
    for (int k = 0; k < cubeSize; k++) {
      for (int j = 0; j < cubeSize; j++) {
        int val =
            Cube[k * cubeSize * cubeSize + (cubeSize - i - 1) * cubeSize + j];
        std::cout << ((val < 10) ? "0" + std::to_string(val)
                                 : std::to_string(val))
                  << " ";
      }
      std::cout << "|";
    }
    std::cout << std::endl;
  }
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
  unsigned int texIDs[3] = {0, 1, 0};
  unsigned int slots[3] = {0, 1, 0};
  AddShader("shaders/pickingShader2");
  AddShader("shaders/basicShader2");
  add_cube_texture();
  update_animation_speed(0);
  pickedShape = -1;
  SetShapeShader(0, 1);
}

void Assignment3::Update(const Eigen::Matrix4f &Proj,
                         const Eigen::Matrix4f &View,
                         const Eigen::Matrix4f &Model, unsigned int shaderIndx,
                         unsigned int shapeIndx) {
  Shader *s = shaders[shaderIndx];
  int r = ((pickedShape + 1) & 0x000000FF) >> 0;
  int g = ((pickedShape + 1) & 0x0000FF00) >> 8;
  int b = ((pickedShape + 1) & 0x00FF0000) >> 16;
  if (data_list[shapeIndx]->GetMaterial() >= 0 && !materials.empty()) {
    BindMaterial(s, data_list[shapeIndx]->GetMaterial());
  }
  textures[0]->Bind(0);
  s->Bind();
  s->SetUniformMat4f("View", View);
  s->SetUniformMat4f("Model", Model);
  s->SetUniformMat4f("Proj", Proj);
  s->Unbind();
}

void Assignment3::WhenRotate() {}

void Assignment3::WhenTranslate() {}

void Assignment3::Animate() {
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
  case 2: // the 3rd options
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
  std::vector<int> pointers =
      get_face_indices(CUBE_SIZE, face_direction, face_index);
  std::vector<int> positions = rotated_face_indices(
      CUBE_SIZE, face_direction == 2 ? rotate_clockwise : !rotate_clockwise);
  std::vector<int> values;
  for (int i = 0; i < CUBE_SIZE * CUBE_SIZE; i++) {
    values.push_back(cubes_indices[pointers[i]]);
  }
  for (int i = 0; i < CUBE_SIZE * CUBE_SIZE; i++) {
    cubes_indices[pointers[i]] = values[positions[i]];
  }
  Operation operation;
  operation.type = face_direction;
  operation.indices = values;
  printCube(CUBE_SIZE, cubes_indices);
  operations.push(operation);
}

void Assignment3::AddOperation(int opcode) {
  // rotation
  if (opcode < 6) {
    add_rotation(opcode / 2, (opcode % 2) * (CUBE_SIZE - 1));
    return;
  }
  // // inner wall rotation
  // if (opcode >= 10) {
  //   if (cube_size < 4) {
  //     std::cout << "operation is not permitted in cubes sized less than 4"
  //               << std::endl;
  //     return;
  //   }
  //   opcode -= 10;
  //   add_rotation(opcode / 2, (opcode % 2) ? (cube_size - 2) : 1);
  //   return;
  // }
  // others
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
    toggle_rotation_dir();
  else
    update_animation_speed((op.type == 5) ? 1 : -1);
}

void Assignment3::rotate_wall(int type, std::vector<int> indices) {
  curr_frame += anim_speed;
  float amount = ROTATION_DEGREE / TOTAL_FRAMES;
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
    ShapeTransformation(rotate_offset + type, amount, 0);
  }
}

void Assignment3::WhenPicked() {
  // pickedShape = -1;
  std::vector<int>::iterator itr =
      std::find(cubes_indices.begin(), cubes_indices.end(), pickedShape);
  if (itr != cubes_indices.cend()) {
    int cube_index = std::distance(cubes_indices.begin(), itr);
    std::cout << pickedShape << " present at index " << cube_index << std::endl;
    int face_index = cube_index / (CUBE_SIZE * CUBE_SIZE);
    cube_index -= face_index * CUBE_SIZE * CUBE_SIZE;
    int row_index = cube_index / CUBE_SIZE;
    int col_index = cube_index - row_index * CUBE_SIZE;

    // int pickedShapeNormalMax = (pickedNormal[0] > pickedNormal[1] &&
    // pickedNormal[0] > pickedNormal[2]) ? 0 : (pickedNormal[1] >
    // pickedNormal[2]) ? 1 : 2;

    switch (pickedShapeNormalMax) {
    case 0:
      add_rotation(0, col_index);
      break;
    case 1:
      add_rotation(1, row_index);
      break;
    case 2:
      add_rotation(2, face_index);
    default:
      break;
    }
  } else {
    std::cout << "Element not found";
  }
}

void Assignment3::add_cube_texture() {
  float center = (CUBE_SIZE - 1.0f) / 2.0f;
  int pad = 1;
  for (int i = 0; i < CUBE_SIZE; i++) {
    for (int j = 0; j < CUBE_SIZE; j++) {
      for (int k = 0; k < CUBE_SIZE; k++) {
        AddShape(Cube, -1, TRIANGLES);
        pickedShape = i * CUBE_SIZE * CUBE_SIZE + j * CUBE_SIZE + k;
        ShapeTransformation(xTranslate, pad * (k - center), 0);
        ShapeTransformation(yTranslate, pad * (j - center), 0);
        ShapeTransformation(zTranslate, pad * (i - center), 0);
        cubes_indices.push_back(pickedShape);
      }
    }
  }
  AddTexture("textures/plane.png", 2);
  printCube(CUBE_SIZE, cubes_indices);
}

Assignment3::~Assignment3(void) {}