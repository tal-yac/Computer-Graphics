#include "Project.h"
#include <chrono>
#include <iostream>

bool endsWith(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return false;
  }
}
static void printMat(const Eigen::Matrix4d &mat) {
  std::cout << " matrix:" << std::endl;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      std::cout << mat(j, i) << " ";
    std::cout << std::endl;
  }
}

long getCurrentUnixTime() {
  const auto p1 = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch())
      .count();
}

IGL_INLINE void Project::Draw(int shaderIndx, const Eigen::Matrix4f &Proj,
                              const Eigen::Matrix4f &View, int viewportIndx,
                              unsigned int flgs, unsigned int property_id) {
  if (animationStatus == PLAYING && globalTime < maxTime()) {
    ++globalTime;
    Eigen::Vector3f cameraPos =
        GetCamera(cameraScreenAnimation)->mover->getPosition(globalTime);
    auto delta = cameraPos - prevCamera;
    renderer->MoveCamera(0, xTranslate, delta.x());
    renderer->MoveCamera(0, yTranslate, delta.y());
    renderer->MoveCamera(0, zTranslate, delta.z());
    prevCamera = cameraPos;
  }

  Eigen::Matrix4f Normal;

  if (!(staticScene & (1 << viewportIndx)))
    Normal = MakeTransScale();
  else
    Normal = Eigen::Matrix4f::Identity();

  for (int i = 0; i < data_list.size(); i++) {
    if (isDeleted(i) || (i != backgroundShape[0] && i != backgroundShape[1] &&
                         i != backgroundShape[2] && i != backgroundShape[3] &&
                         !shapesGlobal[i]->isDrawn(globalTime)))
      continue;

    auto shape = data_list[i];
    if (shape->Is2Render(viewportIndx)) {

      Eigen::Matrix4f Model = shape->MakeTransScale();

      if (!shape->IsStatic()) {

        Model = Normal *
                GetPriviousTrans(View.cast<double>(), i).cast<float>() * Model;
      } else if (parents[i] == -2) {
        Model = View.inverse() * Model;
      }
      if (!(flgs & 65536)) {
        Update(Proj, View, Model, shape->GetShader(), i);
        // Draw fill
        if (shape->show_faces & property_id)
          shape->Draw(shaders[shape->GetShader()], true);
        if (shape->show_lines & property_id) {
          glLineWidth(shape->line_width);
          shape->Draw(shaders[shape->GetShader()], false);
        }
        // overlay draws
        if (shape->show_overlay & property_id) {
          if (shape->show_overlay_depth & property_id)
            glEnable(GL_DEPTH_TEST);
          else
            glDisable(GL_DEPTH_TEST);
          if (shape->lines.rows() > 0) {
            Update_overlay(Proj, View, Model, i, false);
            glEnable(GL_LINE_SMOOTH);
            shape->Draw_overlay(overlay_shader, false);
          }
          if (shape->points.rows() > 0) {
            Update_overlay(Proj, View, Model, i, true);
            shape->Draw_overlay_pints(overlay_point_shader, false);
          }
          glEnable(GL_DEPTH_TEST);
        }
      } else {              // picking
        if (flgs & 16384) { // stencil
          Eigen::Affine3f scale_mat = Eigen::Affine3f::Identity();
          scale_mat.scale(Eigen::Vector3f(1.1f, 1.1f, 1.1f));
          Update(Proj, View, Model * scale_mat.matrix(), 0, i);
        } else {
          Update(Proj, View, Model, 0, i);
        }
        shape->Draw(shaders[0], true);
      }
    }
  }
  if (globalTime >= maxTime())
    Stop();
}

IGL_INLINE ProjectViewerData *Project::data(int mesh_id /*= -1*/) {
  assert(!data_list.empty() && "data_list should never be empty");
  int index;
  if (mesh_id == -1)
    index = selected_data_index;
  else
    index = mesh_index(mesh_id);

  assert((index >= 0 && index < data_list.size()) &&
         "selected_data_index or mesh_id should be in bounds");
  return (ProjectViewerData *)data_list[index];
}

IGL_INLINE const ProjectViewerData *Project::data(int mesh_id /*= -1*/) const {
  assert(!data_list.empty() && "data_list should never be empty");
  int index;
  if (mesh_id == -1)
    index = selected_data_index;
  else
    index = mesh_index(mesh_id);

  assert((index >= 0 && index < data_list.size()) &&
         "selected_data_index or mesh_id should be in bounds");
  return (ProjectViewerData *)data_list[index];
}

IGL_INLINE int Project::append_mesh(bool visible) {
  assert(data_list.size() >= 1);
  data_list.emplace_back(new ProjectViewerData());
  selected_data_index = data_list.size() - 1;
  data_list.back()->id = next_data_id++;
  return data_list.back()->id;
}
Project::Project(igl::opengl::glfw::imgui::ImGuiMenu *menu)
    : igl::opengl::glfw::Viewer(), menu(menu) {
  delete data_list.front();
  data_list.front() = new ProjectViewerData();
}

// Project::Project(float angle ,float relationWH, float near, float far) :
// Scene(angle,relationWH,near,far)
//{
// }

std::shared_ptr<SceneShape>
Project::AddGlobalShape(std::string name, std::string file,
                        std::shared_ptr<ObjectMoverSplit> mover,
                        std::shared_ptr<Layer> layer, std::string shader) {
  int indexes[4];
  std::string fullShapeFile = SHAPES_FOLDER + file + ".obj";
  for (int i = 0; i < 4; i++) {
    indexes[i] = AddShapeFromFile(fullShapeFile, -1, TRIANGLES, i);
  }
  std::shared_ptr<SceneShape> scnShape =
      std::make_shared<SceneShape>(name, file, mover, layer, indexes);
  layer->addShape(scnShape);
  scnShape->shader = GetShader(shader)->getId();
  scnShape->material = 0;
  for (int i = 0; i < 4; i++) {
    shapesGlobal[indexes[i]] = scnShape;
  }
  return scnShape;
}

std::shared_ptr<SceneShape>
Project::AddGlobalShape(std::string name,
                        igl::opengl::glfw::Viewer::shapes shapeType,
                        std::shared_ptr<ObjectMoverSplit> mover,
                        std::shared_ptr<Layer> layer, std::string shader) {
  int indexes[4];
  for (int i = 0; i < 4; i++) {
    indexes[i] = AddShape(shapeType, -1, TRIANGLES, i);
  }
  std::shared_ptr<SceneShape> scnShape =
      std::make_shared<SceneShape>(name, shapeType, mover, layer, indexes);
  layer->addShape(scnShape);
  scnShape->shader = GetShader(shader)->getId();
  scnShape->material = 0;
  for (int i = 0; i < 4; i++) {
    shapesGlobal[indexes[i]] = scnShape;
  }
  return scnShape;
}

void Project::Init(float width, float height) {
  lastFileSystemRefreshingTimeSeconds = 0;
  resolution = Eigen::Vector2f(width, height);
  globalTime = 0;
  controlledCamera = MAIN;
  mouseStatus = NOT_PRESSED;
  shouldDrawPickingRectangle = false;
  unsigned int texIDs[3] = {1, 2, 3};
  unsigned int slots[3] = {1, 2, 3};

  AddShader("shaders/pickingShader");
  AddShader("shaders/cubemapShader");
  AddShader("shaders/basicShader");
  AddShader("shaders/basicShader2");

  AddTexture("textures/grass.bmp", 2);
  AddTexture("textures/cubemaps/Daylight Box_", 3);
  AddTexture("textures/box0.bmp", 2);

  int mat1 = AddMaterial(texIDs, slots, 1);
  int mat2 = AddMaterial(texIDs + 1, slots + 1, 1);
  int mat3 = AddMaterial(texIDs + 2, slots + 2, 1);
  int mat4 = AddMaterial(texIDs + 3, slots + 3, 1);

  for (size_t i = 0; i < 4; i++) {
    backgroundShape[i] = AddShape(Sphere, -1, TRIANGLES, i);
    selected_data_index = backgroundShape[i];
    ShapeTransformation(scaleAll, 1000, 0);
    SetShapeStatic(backgroundShape[i]);
  }

  std::vector<Eigen::Vector3f> points = {
      Eigen::Vector3f(0, 0, 0), Eigen::Vector3f(0, 20, 0),
      Eigen::Vector3f(-10, -10, -100), Eigen::Vector3f(0, 0, 0)};

  std::vector<Eigen::Vector3f> pointsRev = {
      Eigen::Vector3f(0, 0, 0), Eigen::Vector3f(-10, -10, -100),
      Eigen::Vector3f(0, 20, 0), Eigen::Vector3f(0, 0, 0)};

  std::shared_ptr<ObjectMoverBezier> bez =
      std::make_shared<ObjectMoverBezier>(points, 0, 500);
  auto constMover =
      std::make_shared<ObjectMoverConstant>(Eigen::Vector3f(0, 0, 0), 500, 50);
  std::shared_ptr<ObjectMoverBezier> bezRev =
      std::make_shared<ObjectMoverBezier>(pointsRev, 550, 500);

  std::vector<std::shared_ptr<ObjectMover>> movers = {bez, constMover, bezRev};
  AddMovementCurve(std::make_shared<ObjectMoverSplit>(movers, "default"));

  std::shared_ptr<ObjectMoverSplit> mover = GetCurve("default");
  auto defaultLayer = layerManager.addLayer("default");

  auto constZeroMover =
      std::make_shared<ObjectMoverConstant>(Eigen::Vector3f(0, 0, 0), 0, 1050);
  AddMovementCurve(std::make_shared<ObjectMoverSplit>(
      std::vector<std::shared_ptr<ObjectMover>>({constZeroMover}),
      "constZero"));
  AddGlobalCamera("default", Eigen::Vector3d(0, 0, 0), GetCurve("constZero"));
  cameraScreen1 = "default";
  cameraScreen2 = "default";
  cameraScreen1Position = Eigen::Vector3d(0, 0, 0);
  cameraScreenAnimation = "default";
  cameraScreen2Position = Eigen::Vector3d(0, 0, 0);

  SetBackgroundShader("basicShader1");

  splitCameraOption = UNSPLIT;
  prevSplitCameraOption = UNSPLIT;
  animationStatus = STOPPED;
}

float Project::maxTime() {
  float maxTime = -1;
  for (std::shared_ptr<SceneShape> s : getAllShapes()) {
    maxTime = std::max(maxTime, s->getEndTime());
  }
  return maxTime;
}

void Project::CalculateShapePosition(int shapeIndex) {
  std::shared_ptr<SceneShape> scnShape = shapesGlobal[shapeIndex];
  selected_data_index = shapeIndex;
  Eigen::Vector3f pos = scnShape->getlastDrawnPosition(shapeIndex);
  Eigen::Vector3f newPos = scnShape->getPosition((float)globalTime);
  Eigen::Vector3f delta = newPos - pos;
  ShapeTransformation(xTranslate, delta[0], 0);
  ShapeTransformation(yTranslate, delta[1], 0);
  ShapeTransformation(zTranslate, delta[2], 0);
  shapesGlobal[shapeIndex]->setlastDrawnPosition(newPos, shapeIndex);
}

void Project::CalculateShapeSize(int shapeIndex) {
  std::shared_ptr<SceneShape> scnShape = shapesGlobal[shapeIndex];
  if (!scnShape->NeedsRescale(shapeIndex))
    return;
  selected_data_index = shapeIndex;
  ShapeTransformation(scaleAll, scnShape->GetNormalizedScale(), 0);
  scnShape->MarkAsRescaled(shapeIndex);
}

void Project::Update(const Eigen::Matrix4f &Proj, const Eigen::Matrix4f &View,
                     const Eigen::Matrix4f &Model, unsigned int shaderIndx,
                     unsigned int shapeIndx) {
  int r = ((shapeIndx + 1) & 0x000000FF) >> 0;
  int g = ((shapeIndx + 1) & 0x0000FF00) >> 8;
  int b = ((shapeIndx + 1) & 0x00FF0000) >> 16;
  Shader *s = shaders[shaderIndx];
  if (shapeIndx != backgroundShape[0] && shapeIndx != backgroundShape[1] &&
      shapeIndx != backgroundShape[2] && shapeIndx != backgroundShape[3]) {
    std::shared_ptr<SceneShape> scnShape = shapesGlobal[shapeIndx];
    SetShapeShader(shapeIndx, scnShape->shader);
    SetShapeMaterial(shapeIndx, scnShape->material);
    CalculateShapePosition(shapeIndx);
    CalculateShapeSize(shapeIndx);
    auto posInWorld = scnShape->getPosition(globalTime);
    auto pos4DInWorld = Eigen::Vector4f(posInWorld[0], posInWorld[1], 0, 1);
    Eigen::Vector4f locationOnScreen4d =
        Proj.transpose() * View.transpose() * Model.transpose() * pos4DInWorld;

    scnShape->locationsOnScreen[shapeIndx] =
        Eigen::Vector2f(locationOnScreen4d[0], locationOnScreen4d[1]);
  }

  s->Bind();
  s->SetUniformMat4f("Proj", Proj);
  s->SetUniformMat4f("View", View);
  s->SetUniformMat4f("Model", Model);
  if (createdShadersById.find(shaderIndx) != createdShadersById.end())
    createdShadersById[shaderIndx]->uploadAllUniforms(globalTime, resolution,
                                                      mousePos);
  else if (backgroundShader != nullptr &&
           backgroundShader->getId() == shaderIndx) {
    backgroundShader->uploadAllUniforms(globalTime, resolution, mousePos);
  }
  if (data_list[shapeIndx]->GetMaterial() >= 0 && !materials.empty()) {
    BindMaterial(s, data_list[shapeIndx]->GetMaterial());
  }
  if (shaderIndx == 0)
    s->SetUniform4f("lightColor", r / 255.0f, g / 255.0f, b / 255.0f, 0.0f);
  else
    s->SetUniform4f("lightColor", 4 / 100.0f, 60 / 100.0f, 99 / 100.0f, 0.5f);
  s->Unbind();
}

void Project::WhenRotate() {}

void Project::WhenTranslate() {}

void Project::Animate() {

  if (isActive) {
    if (selected_data_index > 0)
      data()->MyRotate(Eigen::Vector3d(0, 1, 0), 0.01);
  }
}

void Project::ScaleAllShapes(float amt, int viewportIndx) {
  for (int i = 1; i < data_list.size(); i++) {
    if (data_list[i]->Is2Render(viewportIndx)) {
      data_list[i]->MyScale(Eigen::Vector3d(amt, amt, amt));
    }
  }
}

Project::~Project(void) { delete renderer; }

void Project::SetRenderer(Renderer *renderer) {
  if (this->renderer != nullptr)
    throw std::invalid_argument("renderer cannot be set twice");
  this->renderer = renderer;
}

void Project::SetParent(int shape, int newParent) {
  shapesGlobal[shapesGlobal[shape]->getParent()]->removeChild(shape);
  shapesGlobal[shape]->setParent(newParent);
  shapesGlobal[newParent]->addChild(shape);
}

int Project::GetParent(int shape) { return shapesGlobal[shape]->getParent(); }
long Project::GetGlobalTime() { return globalTime; }

std::vector<int> Project::GetChildren(int shape) {
  return shapesGlobal[shape]->getChildren();
}

AnimationStatus Project::getAnimationStatus() { return animationStatus; }

void Project::Play() {
  previousState = GetSplitCameraOption();
  prevCamera = Eigen::Vector3f::Zero();
  animationStatus = PLAYING;
  ResetRenderer(MAIN, UNSPLIT, GetCamera(cameraScreenAnimation)->GetPosition(),
                cameraScreen2Position);
}

void Project::Pause() {
  animationStatus = PAUSED;
  switch (previousState) {
  case UNSPLIT:
    Unsplit();
    break;
  case SPLITX:
    SplitX();
    break;
  case SPLITY:
    SplitY();
    break;
  }
}

void Project::Stop() {
  globalTime = 0;
  animationStatus = STOPPED;
  switch (previousState) {
  case UNSPLIT:
    Unsplit();
    break;
  case SPLITX:
    SplitX();
    break;
  case SPLITY:
    SplitY();
    break;
  }
}

void Project::Replay() {
  globalTime = 0;
  animationStatus = PLAYING;
}

void Project::SetGlobalTime(long time) { globalTime = time; }

void Project::UpdateWindowLocation(ImVec2 topLeft, ImVec2 bottomRight) {
  windowLocation.topLeft = topLeft;
  windowLocation.bottomRight = bottomRight;
}

std::shared_ptr<SceneShader> Project::GetShader(const std::string &shaderName) {
  if (createdShadersByName.find(shaderName) != createdShadersByName.end())
    return createdShadersByName[shaderName];
  std::string shaderPath = SHADERS_FOLDER + shaderName;
  int shaderId = AddShader(shaderPath);
  std::shared_ptr<SceneShader> shader = nullptr;
  if (shaderId != -1) {
    if (AddGlobalShader(std::make_shared<SceneShader>(
            shaderName, shaderId, shaders[shaderId], SHADERS_FOLDER)))
      shader = createdShadersById[shaderId];
  }
  return shader;
}
std::shared_ptr<SceneShader> Project::GetShader(const int shaderId) {
  if (createdShadersById.find(shaderId) == createdShadersById.end())
    return nullptr;
  return createdShadersById[shaderId];
}

std::vector<std::string> Project::GetAllShaders() {
  long currentTime = getCurrentUnixTime();
  if (currentTime - lastFileSystemRefreshingTimeSeconds > 60) {
    RefreshShadersList();
    RefreshShapeFilesList();
    lastFileSystemRefreshingTimeSeconds = currentTime;
  }

  return allShaders;
}

std::vector<std::string> Project::GetAllShapeFiles() {
  long currentTime = getCurrentUnixTime();
  if (currentTime - lastFileSystemRefreshingTimeSeconds > 60) {
    RefreshShadersList();
    RefreshShapeFilesList();
    lastFileSystemRefreshingTimeSeconds = currentTime;
  }
  return allShapeFiles;
}

void Project::RefreshShapeFilesList() {
  allShapeFiles.clear();
  for (auto const &file : filesystem::directory_iterator(SHAPES_FOLDER)) {
    std::string path;
#if USING_BOOST
    path = file.path().string();
#else
    path = file.path().u8string();
#endif
    if (endsWith(path, ".obj")) {
      path = path.substr(0, path.find_last_of('.'));
      path = path.substr(SHAPES_FOLDER.length(), path.length());
      allShapeFiles.push_back(path);
    }
  }
  std::sort(allShapeFiles.begin(), allShapeFiles.end());
}

void Project::RefreshShadersList() {
  allShaders.clear();
  for (auto const &file : filesystem::directory_iterator(SHADERS_FOLDER)) {
    std::string path;
#if USING_BOOST
    path = file.path().string();
#else
    path = file.path().u8string();
#endif
    if (endsWith(path, ".glsl")) {
      path = path.substr(0, path.find_last_of('.'));
      path = path.substr(SHADERS_FOLDER.length(), path.length());
      allShaders.push_back(path);
    }
  }
  std::sort(allShaders.begin(), allShaders.end());
}

std::vector<std::shared_ptr<SceneShape>> Project::getAllShapes() {
  std::vector<std::shared_ptr<SceneShape>> allShapes;
  for (auto entry : shapesGlobal) {
    if (std::find(allShapes.begin(), allShapes.end(), entry.second) ==
        allShapes.end())
      allShapes.push_back(entry.second);
  }
  return allShapes;
}

std::shared_ptr<SceneShape> Project::GetGlobalShape(const std::string &name) {
  for (auto &shape : getAllShapes()) {
    if (shape->name == name)
      return shape;
  }
  return nullptr;
}

std::string Project::GetShaderName(int shaderId) {
  return (shaderId == -1) ? "" : createdShadersById[shaderId]->getName();
}

int Project::GetShaderId(std::string shaderName) {
  auto shader = GetShader(shaderName);
  return (shader == nullptr) ? -1 : shader->getId();
}

bool Project::AddGlobalShader(std::shared_ptr<SceneShader> shader) {
  if (createdShadersById.find(shader->getId()) != createdShadersById.end() ||
      createdShadersByName.find(shader->getName()) !=
          createdShadersByName.end())
    return false;

  createdShadersByName[shader->getName()] = shader;
  createdShadersById[shader->getId()] = shader;
  return true;
}

std::shared_ptr<SceneCamera> Project::GetCamera(const std::string &cameraName) {
  return createdCamerasByName.at(cameraName);
}

std::vector<std::string> Project::GetAllCameras() { return allCameras; }

std::string Project::GetCameraScreen1() { return cameraScreen1; }

std::string Project::GetCameraScreen2() { return cameraScreen2; }

std::string Project::GetCameraScreenAnimation() {
  return cameraScreenAnimation;
}

std::shared_ptr<SceneCamera>
Project::AddGlobalCamera(std::string _name, Eigen::Vector3d _pos,
                         std::shared_ptr<ObjectMoverSplit> _mover) {

  std::shared_ptr<SceneCamera> scnCamera =
      std::make_shared<SceneCamera>(_name, -1, _pos, _mover);

  allCameras.push_back(scnCamera->GetName());
  std::sort(allCameras.begin(), allCameras.end());
  createdCamerasByName.insert({scnCamera->GetName(), scnCamera});

  return scnCamera;
}

void Project::UpdateResolution(float width, float height) {
  resolution = Eigen::Vector2f(width, height);
}

float distance(Eigen::Vector2f first, Eigen::Vector2f second) {
  return sqrt(pow(first[0] - second[0], 2) + pow(first[1] - second[1], 2));
}

void Project::UpdateMouse(float x, float y) {
  mousePos = Eigen::Vector2f(x, y);

  if ((x >= windowLocation.topLeft.x && x <= windowLocation.bottomRight.x &&
       y >= windowLocation.topLeft.y && y <= windowLocation.bottomRight.y) ||
      animationStatus == PLAYING)
    return;

  if (mouseStatus == LEFT_CLICK &&
      distance(mousePos, pressStartPosition) > 0.01) {
    float xAngle = 1.2f * (pressStartPosition[0] - x) / resolution[0];
    float yAngle = 1.2f * (pressStartPosition[1] - y) / resolution[1];

    renderer->MoveCamera(GetConrolledCameraId(), yRotate, xAngle);
    renderer->MoveCamera(GetConrolledCameraId(), xRotate, yAngle);
    pressStartPosition = mousePos;
  } else if (mouseStatus == RIGHT_CLICK && x != pressStartPosition.x() &&
             y != pressStartPosition.y()) {
    UpdatePickingRectangle(
        BoundingRectangle(Eigen::Vector2f(x, y), pressStartPosition));
  }
}

void Project::SetBackgroundShader(std::string shaderName) {
  backgroundShader = GetShader(shaderName);
  for (size_t i = 0; i < 4; i++) {
    SetShapeShader(backgroundShape[i], backgroundShader->getId());
  }
}

std::string Project::GetBackgroundShader() {
  return (backgroundShader == nullptr) ? "" : backgroundShader->getName();
}

std::vector<std::string> Project::GetAllMovementCurves() {
  std::vector<std::string> curves;
  for (auto &entry : movementCurves)
    curves.push_back(entry.first);
  return curves;
}

bool Project::AddMovementCurve(std::shared_ptr<ObjectMoverSplit> curve) {
  if (movementCurves.find(curve->name) != movementCurves.end())
    return false;
  movementCurves[curve->name] = curve;
  return true;
}

void Project::SetNewMoversForCurve(
    std::string name, std::vector<std::shared_ptr<ObjectMover>> newMovers) {
  movementCurves[name]->movers = newMovers;
}

std::shared_ptr<ObjectMoverSplit> Project::GetCurve(std::string name) {
  return (movementCurves.find(name) == movementCurves.end())
             ? nullptr
             : movementCurves[name];
}

void Project::SetShapeCurve(int shapeId, std::string curveName) {
  shapesGlobal[shapeId]->mover = movementCurves[curveName];
}

Eigen::Vector2f Project::GetMouse() { return mousePos; }

void Project::SetDisplay(Display *display) { this->display = display; }

void Project::SetSplitCameraOption(SplitCameraOption cameraOption) {
  splitCameraOption = cameraOption;
}

SplitCameraOption Project::GetSplitCameraOption() { return splitCameraOption; }

SplitCameraOption Project::GetPrevSplitCameraOption() {
  return prevSplitCameraOption;
}

void Project::SetPrevSplitCameraOption(SplitCameraOption prevCameraOption) {
  prevSplitCameraOption = prevCameraOption;
}

void Project::MoveCamera(Renderer::transformations transformation, float amt) {
  switch (transformation) {
  case Renderer::xRotate:
    if (controlledCamera == MAIN || controlledCamera == TOP ||
        controlledCamera == LEFT) {
      cameraScreen1Position += Eigen::Vector3d(0, amt, 0);
    } else {
      cameraScreen2Position += Eigen::Vector3d(0, amt, 0);
    }
    break;
  case yRotate:
    if (controlledCamera == MAIN || controlledCamera == TOP ||
        controlledCamera == LEFT) {
      cameraScreen1Position += Eigen::Vector3d(amt, 0, 0);
    } else {
      cameraScreen2Position += Eigen::Vector3d(amt, 0, 0);
    }
    break;
  case zRotate:
    if (controlledCamera == MAIN || controlledCamera == TOP ||
        controlledCamera == LEFT) {
      cameraScreen1Position += Eigen::Vector3d(0, 0, amt);
    } else {
      cameraScreen2Position += Eigen::Vector3d(0, 0, amt);
    }
    break;
  }
  std::cout << "updated Position 1:" << cameraScreen1Position[0] << std::endl;
  renderer->MoveCamera(GetConrolledCameraId(), transformation, amt);
}

void Project::ResetRenderer(ControlledCamera contCam,
                            SplitCameraOption camSplit,
                            Eigen::Vector3d posCamera1,
                            Eigen::Vector3d posCamera2) {
  std::list<int> x, y;

  auto oldRenderer = renderer;
  float CAMERA_ANGLE = 45.0f;
  const float NEAR = 1.0f;

  renderer = new Renderer(
      CAMERA_ANGLE, (float)resolution[0] / (float)resolution[1], NEAR, FAR);

  renderer->AddCamera(Eigen::Vector3d(0, 0, 0), 45,
                      (float)resolution[0] / (float)resolution[1], NEAR, FAR,
                      1);
  renderer->AddCamera(Eigen::Vector3d(0, 0, 0), 45,
                      (float)resolution[0] / (float)resolution[1], NEAR, FAR,
                      2);
  renderer->AddCamera(Eigen::Vector3d(0, 0, 0), 45,
                      (float)resolution[0] / (float)resolution[1], NEAR, FAR,
                      3);

  if (camSplit == SPLITX) {
    x.push_back(resolution[0] / 2);
    x.push_back(resolution[0]);
    y.push_back(resolution[1] - 1);
    y.push_back(resolution[1]);
    renderer->MoveCamera(0, yRotate, posCamera1[0]);
    renderer->MoveCamera(0, xRotate, posCamera1[1]);
    renderer->MoveCamera(0, zRotate, posCamera1[2]);
    renderer->MoveCamera(2, yRotate, posCamera2[0]);
    renderer->MoveCamera(2, xRotate, posCamera2[1]);
    renderer->MoveCamera(2, zRotate, posCamera2[2]);
  }

  else if (camSplit == SPLITY) {
    x.push_back(resolution[0] - 1);
    x.push_back(resolution[0]);
    y.push_back(resolution[1] / 2);
    y.push_back(resolution[1]);
    renderer->MoveCamera(1, yRotate, posCamera1[0]);
    renderer->MoveCamera(1, xRotate, posCamera1[1]);
    renderer->MoveCamera(1, zRotate, posCamera1[2]);
    renderer->MoveCamera(0, yRotate, posCamera2[0]);
    renderer->MoveCamera(0, xRotate, posCamera2[1]);
    renderer->MoveCamera(0, zRotate, posCamera2[2]);
  }

  else if (camSplit == UNSPLIT) {
    x.push_back(resolution[0] - 1);
    x.push_back(resolution[0]);
    y.push_back(resolution[1] - 1);
    y.push_back(resolution[1]);
    renderer->MoveCamera(0, yRotate, posCamera1[0]);
    renderer->MoveCamera(0, xRotate, posCamera1[1]);
    renderer->MoveCamera(0, zRotate, posCamera1[2]);
  }
  renderer->Init(this, x, y, 1, menu);

  display->SetRenderer(renderer);
  delete oldRenderer;
  controlledCamera = contCam;
  cameraScreen1Position = posCamera1;
  cameraScreen2Position = posCamera2;
  SetSplitCameraOption(camSplit);
  display->launch_rendering(renderer);
}

void Project::SplitX() {
  ResetRenderer(LEFT, SPLITX, cameraScreen1Position, cameraScreen2Position);
}

void Project::SplitY() {
  ResetRenderer(TOP, SPLITY, cameraScreen1Position, cameraScreen2Position);
}

void Project::Unsplit() {
  ResetRenderer(MAIN, UNSPLIT, cameraScreen1Position, cameraScreen2Position);
}

void Project::SetCameraScreen1(std::string cameraName) {
  cameraScreen1 = cameraName;
  auto posCamera1 = GetCamera(cameraScreen1)->GetPosition();

  switch (splitCameraOption) {
  case UNSPLIT:
    ResetRenderer(MAIN, UNSPLIT, posCamera1, cameraScreen2Position);
    break;
  case SPLITX:
    ResetRenderer(LEFT, SPLITX, posCamera1, cameraScreen2Position);
    break;
  case SPLITY:
    ResetRenderer(TOP, SPLITY, posCamera1, cameraScreen2Position);
    break;
  }
}

void Project::SetCameraScreen2(std::string cameraName) {
  cameraScreen2 = cameraName;
  auto posCamera2 = GetCamera(cameraScreen2)->GetPosition();

  switch (splitCameraOption) {
  case UNSPLIT:
    ResetRenderer(MAIN, UNSPLIT, cameraScreen1Position, posCamera2);
    break;
  case SPLITX:
    ResetRenderer(LEFT, SPLITX, cameraScreen1Position, posCamera2);
    break;
  case SPLITY:
    ResetRenderer(TOP, SPLITY, cameraScreen1Position, posCamera2);
    break;
  }
}

void Project::SetCameraScreenAnimation(std::string cameraName) {
  cameraScreenAnimation = cameraName;
}

bool Project::isDeleted(int id) {
  return deletedShapes.find(id) != deletedShapes.end();
}

void Project::DeleteShape(std::shared_ptr<SceneShape> shape) {
  shape->getLayer()->deleteShape(shape);
  for (int id : shape->GetIds()) {
    shapesGlobal.erase(id);
    deletedShapes[id] = true;
  }
}

void Project::SetViewportWidth(int w) { this->VP_Width = w; }
void Project::SetViewportHeight(int w) { this->VP_Height = w; }

void Project::ChangeControlledCamera() {
  switch (splitCameraOption) {
  case SPLITX:
    controlledCamera = (controlledCamera == LEFT) ? RIGHT : LEFT;
    break;
  case SPLITY:
    controlledCamera = (controlledCamera == TOP) ? BOTTOM : TOP;
  }
}

int Project::GetConrolledCameraId() {
  switch (controlledCamera) {
  case MAIN:
  case BOTTOM:
  case LEFT:
    return 0;
  case TOP:
    return 1;
  case RIGHT:
    return 2;
  }
}

void Project::RightClick(float x, float y) { mouseStatus = RIGHT_CLICK; }

void Project::LeftClick(float x, float y) {
  multiPickedShapes.clear();
  shouldDrawPickingRectangle = true;
  pressStartPosition = Eigen::Vector2f(x, y);
  mouseStatus = LEFT_CLICK;
}

void Project::UnpressMouse(float x, float y) {
  switch (mouseStatus) {
  case LEFT_CLICK:
    HandleLeftClickEnd(x, y);
    break;
  case RIGHT_CLICK:
    HandleRightClickEnd(x, y);
    break;
  case NOT_PRESSED:
    throw std::runtime_error("Mouse was not pressed - cannot unpress!");
    break;
  }

  mouseStatus = NOT_PRESSED;
}

bool Project::IsMousePressed() { return mouseStatus != NOT_PRESSED; }

void Project::HandleLeftClickEnd(float x, float y) {}

void Project::HandleRightClickEnd(float x, float y) {
  if (x == pressStartPosition.x() || y == pressStartPosition.y())
    return;
  shouldDrawPickingRectangle = false;
}

void Project::OpenNewWindow(std::shared_ptr<GuiState> state) {
  guiStates.push(state);
}

void Project::CloseCurrentWindow() { guiStates.pop(); }

bool Project::IsGuiInitialized() const { return !guiStates.empty(); }

std::shared_ptr<GuiState> Project::GetCurrentWindow() {
  return guiStates.top();
}

void Project::UpdatePickingRectangle(BoundingRectangle rec) {
  shouldDrawPickingRectangle = true;
  pickingRectangle = std::move(rec);
}

std::vector<std::shared_ptr<SceneShape>> Project::GetPickedShapes() {
  return multiPickedShapes;
}
