#include "Assignment3.h"
#include "external\glad\include\glad\glad.h"
#include <iostream>

static constexpr int MAX_ANIMATIONS = 20;
static constexpr int TOTAL_FRAMES = 5 * 11;
static constexpr int CUBE_TEXTURE_SIZE = 870;

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
  anim_speed = 1;
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

  AddCubeTexture();

  float center = (cube_size - 1.0f) / 2.0f;
  for (int i = 0; i < cube_size; i++) {
    for (int j = 0; j < cube_size; j++) {
      for (int k = 0; k < cube_size; k++) {
        AddShape(Cube, -1, TRIANGLES);
        pickedShape = i * cube_size * cube_size + j * cube_size + k;
        ShapeTransformation(xTranslate, 1 * (k - center), 0);
        ShapeTransformation(yTranslate, 1 * (j - center), 0);
        ShapeTransformation(zTranslate, 1 * (i - center), 0);

        cubesIndexs.push_back(pickedShape);
      }
    }
  }
  UpdateAnimationSpeed(0);
  pickedShape = -1;
  SetShapeShader(0, 1);

  // SetShapeViewport(6, 1);
  //	ReadPixel(); //uncomment when you are reading from the z-buffer
}

void Assignment3::Update(const Eigen::Matrix4f &Proj,
                         const Eigen::Matrix4f &View,
                         const Eigen::Matrix4f &Model, unsigned int shaderIndx,
                         unsigned int shapeIndx) {
  // 	Shader *s = shaders[shaderIndx];
  // 	int r = ((shapeIndx+1) & 0x000000FF) >>  0;
  // 	int g = ((shapeIndx+1) & 0x0000FF00) >>  8;
  // 	int b = ((shapeIndx+1) & 0x00FF0000) >> 16;

  // 		s->Bind();
  // 	s->SetUniformMat4f("Proj", Proj);
  // 	s->SetUniformMat4f("View", View);
  // 	s->SetUniformMat4f("Model", Model);
  // 	if (data_list[shapeIndx]->GetMaterial() >= 0 && !materials.empty())
  // 	{
  // //		materials[shapes[pickedShape]->GetMaterial()]->Bind(textures);
  // 		BindMaterial(s, data_list[shapeIndx]->GetMaterial());
  // 	}
  // 	if (shaderIndx == 0)
  // 		s->SetUniform4f("lightColor", r / 255.0f, g / 255.0f, b /
  // 255.0f,
  // 0.0f); 	else 		s->SetUniform4f("lightColor", 4/100.0f, 60 /
  // 100.0f, 99 / 100.0f, 0.5f);
  // 	//textures[0]->Bind(0);

  // 	//s->SetUniform1i("sampler2",
  // materials[shapes[pickedShape]->GetMaterial()]->GetSlot(1));
  // 	//s->SetUniform4f("lightDirection", 0.0f , 0.0f, -1.0f, 0.0f);
  // //	if(shaderIndx == 0)
  // //		s->SetUniform4f("lightColor",r/255.0f, g/255.0f, b/255.0f,1.0f);
  // //	else
  // //		s->SetUniform4f("lightColor",0.7f,0.8f,0.1f,1.0f);
  // 	s->Unbind();
  Shader *s = shaders[shaderIndx];
  int r = ((pickedShape + 1) & 0x000000FF) >> 0;
  int g = ((pickedShape + 1) & 0x0000FF00) >> 8;
  int b = ((pickedShape + 1) & 0x00FF0000) >> 16;
  if (data_list[shapeIndx]->GetMaterial() >= 0 && !materials.empty()) {
    // materials[shapes[pickedShape]->GetMaterial()]->Bind(textures);
    BindMaterial(s, data_list[shapeIndx]->GetMaterial());
  }
  textures[0]->Bind(0);
  s->Bind();
  s->SetUniformMat4f("View", View);
  s->SetUniformMat4f("Model", Model);
  s->SetUniformMat4f("Proj", Proj);
  // s->SetUniform1i("sampler1",
  // materials[shapes[pickedShape]->GetMaterial()]->GetSlot(0)); if (shaderIndx
  // != 2) 	s->SetUniform1i("sampler2",
  // materials[shapes[pickedShape]->GetMaterial()]->GetSlot(1));
  //   s->SetUniform4f("lightColor", r / 255.0, 0, 0, 0);
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

void Assignment3::toggleRotationDir() { rotate_clockwise = !rotate_clockwise; }

void Assignment3::UpdateAnimationSpeed(int change) {
  anim_speed += change;
  if (anim_speed < 1)
    anim_speed = 1;
  else if (anim_speed > MAX_ANIMATIONS)
    anim_speed = MAX_ANIMATIONS;
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

void Assignment3::AddOperation(int operation) {
  if (operation < 6) {
    addRotation(operation / 2, (operation % 2) * (cube_size - 1));
  } // rotation
  else if (operation >= 10) {
    if (cube_size > 3) {
      operation -= 10;
      addRotation(operation / 2, (operation % 2) ? (cube_size - 2) : 1);
    } else
      std::cout << "operation is not premitted in cubes sized less then 3"
                << std::endl;
  } // rotationSecond
  else {
    Operation op;
    op.type = operation - 3;
    operations.push(op);
  } // others
}
void Assignment3::ReadOperation() {
  if (operations.empty())
    return;
  Operation op = operations.front();
  if (op.type < 3)
    return rotateWall(op.type, op.indices);
  operations.pop();
  if (op.type == 3)
    toggleRotationDir();
  else
    UpdateAnimationSpeed((op.type - 4.5) * 4);
}

void Assignment3::rotateWall(int type, std::vector<int> indexs) {
  curr_frame += anim_speed;
  float amount = (rotate_clockwise) ? -1.0f : 1.0f;
  if (curr_frame >= TOTAL_FRAMES) {
    amount *= (TOTAL_FRAMES + anim_speed - curr_frame);
    operations.pop();
    curr_frame = 0;
  } else
    amount *= anim_speed;

  for (int i = 0; i < cube_size * cube_size; i++) {
    pickedShape = indexs[i];
    switch (type) {
    case 0:
      ShapeTransformation(xRotate, amount, 0);
      break;
    case 1:
      ShapeTransformation(yRotate, amount, 0);
      break;
    case 2:
      ShapeTransformation(zRotate, amount, 0);
      break;
    }
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

void Assignment3::AddCubeTexture() {
  static unsigned char data[CUBE_TEXTURE_SIZE * CUBE_TEXTURE_SIZE * 4];
  for (auto i = 0; i < CUBE_TEXTURE_SIZE; i++) {
    for (auto j = 0; j < CUBE_TEXTURE_SIZE; j++) {
      bool isBlack = !(i < 40 || j < 40 || i > 800 || j > 800);
      data[(i * CUBE_TEXTURE_SIZE + j) * 4] = isBlack ? 255 : 0;
      data[(i * CUBE_TEXTURE_SIZE + j) * 4 + 1] = isBlack ? 255 : 0;
      data[(i * CUBE_TEXTURE_SIZE + j) * 4 + 2] = isBlack ? 255 : 0;
      data[(i * CUBE_TEXTURE_SIZE + j) * 4 + 3] = 0;
    }
  }
  AddTexture(CUBE_TEXTURE_SIZE, CUBE_TEXTURE_SIZE, data, 0);
}

Assignment3::~Assignment3(void) {}