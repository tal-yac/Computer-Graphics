#include "Assignment3.h"
#include "external\glad\include\glad\glad.h"
#include <iostream>

static constexpr int TOTAL_FRAMES = 100;
static constexpr float rotation_degree = 1.5708;

static void printMat(const Eigen::Matrix4d &mat) {
  std::cout << " matrix:" << std::endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      std::cout << mat(j, i) << " ";
    std::cout << std::endl;
  }
}

Assignment3::Assignment3() {
  cube_size = 3; // maximum of six because code support up to 255 cubes in total
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

  // SetShapeViewport(6, 1);
  //	ReadPixel(); //uncomment when you are reading from the z-buffer
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
  std::cout << "animationSpeed " << anim_speed << std::endl;
}

std::vector<int> getFaceIndexes(int cube_size, int direction, int faceIndex) {
  std::vector<int> indexs;
  switch (direction) {
  case 0: // col
    for (int i = 0; i < cube_size * cube_size; i++) {
      indexs.push_back(cube_size * i + faceIndex);
    }
    break;
  case 1: // row
    for (int j = cube_size - 1; j >= 0; j--) {
      for (int i = cube_size - 1; i >= 0; i--) {
        int index = i * cube_size * cube_size + faceIndex * cube_size;
        indexs.push_back(index + j);
      }
    }
    break;
  case 2: // the 3rd options
    for (int j = cube_size - 1; j >= 0; j--) {
      for (int i = cube_size - 1; i >= 0; i--) {
        int index = faceIndex * cube_size * cube_size;
        indexs.push_back(index + i * cube_size + j);
      }
    }
    break;
  default:
    break;
  }
  return indexs;
}

std::vector<int> rotatedFaceIndexes(int cube_size, bool clockwise) {
  std::vector<int> newPositions;
  if (clockwise) {
    for (int i = 0; i < cube_size; i++) {        // col
      for (int j = cube_size - 1; j >= 0; j--) { // row
        newPositions.push_back(j * cube_size + i);
      }
    }
  } else {
    for (int j = cube_size - 1; j >= 0; j--) { // row
      for (int i = 0; i < cube_size; i++) {    // col
        newPositions.push_back(i * cube_size + j);
      }
    }
  }
  return newPositions;
}

void Assignment3::addRotation(int faceDirection, int faceIndex) {
  std::vector<int> pointers =
      getFaceIndexes(cube_size, faceDirection, faceIndex);
  std::vector<int> positions = rotatedFaceIndexes(
      cube_size, faceDirection == 2 ? rotate_clockwise : !rotate_clockwise);
  std::vector<int> values;
  for (int i = 0; i < cube_size * cube_size; i++) {
    values.push_back(cubesIndexs[pointers[i]]);
  }
  for (int i = 0; i < cube_size * cube_size; i++) {
    cubesIndexs[pointers[i]] = values[positions[i]];
  }

  Operation operation;
  operation.type = faceDirection;
  operation.indices = values;
  // printCube(cube_size, cubesIndexs);
  operations.push(operation);
}

void Assignment3::AddOperation(int opcode) {
  // rotation
  if (opcode < 6) {
    addRotation(opcode / 2, (opcode % 2) * (cube_size - 1));
    return;
  }
  // inner wall rotation
  if (opcode >= 10) {
    if (cube_size < 4) {
      std::cout << "operation is not permitted in cubes sized less than 4"
                << std::endl;
      return;
    }
    opcode -= 10;
    addRotation(opcode / 2, (opcode % 2) ? (cube_size - 2) : 1);
    return;
  }
  // others
  Operation op;
  op.type = opcode - 3;
  operations.push(op);
}

void Assignment3::ReadOperation() {
  if (operations.empty())
    return;
  Operation op = operations.front();
  if (op.type < 3)
    return rotateWall(op.type, op.indices);
  operations.pop();
  if (op.type == 3)
    toggle_rotation_dir();
  else
    update_animation_speed((op.type - 4.5) * 4);
}

void Assignment3::rotateWall(int type, std::vector<int> indexs) {
  curr_frame += anim_speed;
  float amount = rotation_degree / TOTAL_FRAMES;
  if (!rotate_clockwise)
    amount *= -1;
  if (curr_frame >= TOTAL_FRAMES) {
    amount *= (TOTAL_FRAMES + anim_speed - curr_frame);
    operations.pop();
    curr_frame = 0;
  } else
    amount *= anim_speed;

  int rotate_offset = xRotate;
  for (int i = 0; i < cube_size * cube_size; i++) {
    pickedShape = indexs[i];
    ShapeTransformation(rotate_offset + type, amount, 0);
  }
}

void Assignment3::WhenPicked() {
  // pickedShape = -1;
  std::vector<int>::iterator itr =
      std::find(cubesIndexs.begin(), cubesIndexs.end(), pickedShape);
  if (itr != cubesIndexs.cend()) {
    int cubeIndex = std::distance(cubesIndexs.begin(), itr);
    std::cout << pickedShape << " present at index " << cubeIndex << std::endl;
    int faceIndex = cubeIndex / (cube_size * cube_size);
    cubeIndex -= faceIndex * cube_size * cube_size;
    int rowIndex = cubeIndex / cube_size;
    int colIndex = cubeIndex - rowIndex * cube_size;

    // int pickedShapeNormalMax = (pickedNormal[0] > pickedNormal[1] &&
    // pickedNormal[0] > pickedNormal[2]) ? 0 : (pickedNormal[1] >
    // pickedNormal[2]) ? 1 : 2;

    switch (pickedShapeNormalMax) {
    case 0:
      addRotation(0, colIndex);
      break;
    case 1:
      addRotation(1, rowIndex);
      break;
    case 2:
      addRotation(2, faceIndex);
    default:
      break;
    }
  } else {
    std::cout << "Element not found";
  }
}

void Assignment3::add_cube_texture() {
  float center = (cube_size - 1.0f) / 2.0f;
  int pad = 1;
  for (int i = 0; i < cube_size; i++) {
    for (int j = 0; j < cube_size; j++) {
      for (int k = 0; k < cube_size; k++) {
        AddShape(Cube, -1, TRIANGLES);
        pickedShape = i * cube_size * cube_size + j * cube_size + k;
        ShapeTransformation(xTranslate, pad * (k - center), 0);
        ShapeTransformation(yTranslate, pad * (j - center), 0);
        ShapeTransformation(zTranslate, pad * (i - center), 0);
        cubesIndexs.push_back(pickedShape);
      }
    }
  }
  AddTexture("textures/plane.png", 2);
}

Assignment3::~Assignment3(void) {}