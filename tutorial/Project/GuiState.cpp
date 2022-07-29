#include "GuiState.h"
#include "StyleManager.h"

GuiState::GuiState(GuiStatus tag) : tag(tag) {}

void GuiState::OpenErrorWindow(Project *project, std::string error) {
  project->OpenNewWindow(std::make_shared<ErrorMsgState>(error));
}

MenuState::MenuState(float hidpi_scaling_, float pixel_ratio_)
    : GuiState(MENU), hidpi_scaling_(hidpi_scaling_),
      pixel_ratio_(pixel_ratio_) {}

std::map<std::string, SplitCameraOption> CAMERA_SPLIT_OPTIONS = {
    {"Unsplit", UNSPLIT}, {"Split x", SPLITX}, {"Split y", SPLITY}};

std::map<SplitCameraOption, std::string> CAMERA_SPLIT_OPTIONS_REV = {
    {UNSPLIT, "Unsplit"}, {SPLITX, "Split x"}, {SPLITY, "Split y"}};

void MenuState::Run(Project *project,
                    std::vector<igl::opengl::Camera *> &camera,
                    Eigen::Vector4i &viewWindow,
                    std::vector<DrawInfo *> drawInfos, ImFont *font,
                    ImFont *boldFont) {
  bool *p_open = NULL;
  static bool no_titlebar = false;
  static bool no_scrollbar = false;
  static bool no_menu = true;
  static bool no_move = false;
  static bool no_resize = false;
  static bool no_collapse = false;
  static bool no_close = false;
  static bool no_nav = false;
  static bool no_background = false;
  static bool no_bring_to_front = false;

  ImGuiWindowFlags window_flags = 0;
  if (no_titlebar)
    window_flags |= ImGuiWindowFlags_NoTitleBar;
  if (no_scrollbar)
    window_flags |= ImGuiWindowFlags_NoScrollbar;
  if (!no_menu)
    window_flags |= ImGuiWindowFlags_MenuBar;
  if (no_move)
    window_flags |= ImGuiWindowFlags_NoMove;
  if (no_resize)
    window_flags |= ImGuiWindowFlags_NoResize;
  if (no_collapse)
    window_flags |= ImGuiWindowFlags_NoCollapse;
  if (no_nav)
    window_flags |= ImGuiWindowFlags_NoNav;
  if (no_background)
    window_flags |= ImGuiWindowFlags_NoBackground;
  if (no_bring_to_front)
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushFont(font);
  ImGui::Begin("## MAIN WINDOW", p_open, window_flags);
  setTheme();
  ImGui::SetWindowPos(ImVec2((float)0, (float)0), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2((float)0, (float)0), ImGuiCond_Always);
  ImGui::End();
  no_move = true;
  no_resize = true;
  ImGui::Begin("## MAIN WINDOW", p_open, window_flags);
  // Theme
  if (ImGui::CollapsingHeader("Theme", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::BeginCombo("##Theme combo", THEMES_REV[theme].c_str())) {
      for (const auto &option : THEMES) {
        std::string name = option.first;
        Theme themeOption = option.second;
        bool is_selected = themeOption == theme;
        if (ImGui::Selectable(name.c_str(), is_selected)) {
          theme = themeOption;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }

  if (ImGui::CollapsingHeader("Background Shader",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    std::string backgroundShader = project->GetBackgroundShader();
    if (ImGui::BeginCombo("##background shader combo",
                          backgroundShader.c_str())) {
      for (const auto &shader : project->GetAllShaders()) {
        bool isSelected = backgroundShader == shader;
        if (ImGui::Selectable(shader.c_str(), isSelected)) {
          project->SetBackgroundShader(shader);
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }

  if (ImGui::CollapsingHeader("Shapes", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const auto &shp : project->getAllShapes()) {
      ImGui::Checkbox(("## MOVE SHAPE" + shp->name).c_str(), &shp->isSelected);
      ImGui::SameLine();
      if (ImGui::Button(shp->name.c_str()))
        project->OpenNewWindow(std::make_shared<ShapeEditingState>(
            shp, project->GetShader(shp->shader)));
      if (shp->isSelected && std::find(project->multiPickedShapes.begin(),
                                       project->multiPickedShapes.end(),
                                       shp) == project->multiPickedShapes.end())
        project->multiPickedShapes.push_back(shp);
      else if (!shp->isSelected &&
               std::find(project->multiPickedShapes.begin(),
                         project->multiPickedShapes.end(),
                         shp) != project->multiPickedShapes.end()) {
        std::vector<std::shared_ptr<SceneShape>> newMPShapes;
        for (auto shape : project->multiPickedShapes) {
          if (shape != shp)
            newMPShapes.push_back(shape);
        }
        project->multiPickedShapes = newMPShapes;
      }
    }
    if (ImGui::Button("Clear Selection")) {
      for (auto shp : project->multiPickedShapes)
        shp->isSelected = false;
      project->multiPickedShapes.clear();
    }
  }
  if (ImGui::Button("Create a new shape## new shape")) {
    project->OpenNewWindow(std::make_shared<ShapeEditingState>());
  }
  if (ImGui::CollapsingHeader("Search##For Shaders",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputText("##shader search pattern", shadersSearchPattern, 30);
    if (ImGui::Button("Clear ##shadersSearchPattern"))
      shadersSearchPattern = strdup("");
  }
  if (!std::string(shadersSearchPattern).empty())
    ImGui::SetNextTreeNodeOpen(true);

  auto allShaders = project->GetAllShaders();
  std::vector<std::string> filteredShaders;
  std::copy_if(
      allShaders.begin(), allShaders.end(), std::back_inserter(filteredShaders),
      // pred:
      [this](std::string shaderName) {
        auto capitalShaderName = shaderName;
        auto capitalPattern = std::string(shadersSearchPattern);
        std::transform(capitalShaderName.begin(), capitalShaderName.end(),
                       capitalShaderName.begin(), ::toupper);
        std::transform(capitalPattern.begin(), capitalPattern.end(),
                       capitalPattern.begin(), ::toupper);
        // shader name starts with pattern
        return capitalShaderName.rfind(capitalPattern, 0) == 0;
      });

  if (ImGui::CollapsingHeader("Edit Selected Shader")) {
    for (const auto &shader : filteredShaders) {
      if (ImGui::Button((shader + "##SHADER").c_str())) {
        std::shared_ptr<SceneShader> selectedShader =
            project->GetShader(shader);
        if (selectedShader == nullptr) {
          OpenErrorWindow(project, "An unknown error has occurred");
        } else {
          project->OpenNewWindow(
              std::make_shared<ShaderEditingState>(selectedShader));
        }
      }
    }
  }
  auto layers = project->layerManager.layers;
  if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (auto const &layerEntry : layers) {
      std::string layerName = layerEntry.first;
      auto layer = layerEntry.second;
      std::string buttonLabel = "X##" + layerName;
      if (ImGui::Button(buttonLabel.c_str())) {
        if (!project->layerManager.removeLayer(layer)) {
          std::string errorMsg =
              "Failed to remove layer " + layerName +
              ".\r\nPlease make sure it has no shapes and try again.";
          OpenErrorWindow(project, errorMsg);
        }
      }
      ImGui::SameLine();
      bool isShown = !layer->isHidden();
      if (ImGui::Checkbox(layerName.c_str(), &isShown)) {
        layer->changeHidden(!isShown);
      }
    }
    ImGui::InputText("##NEW LAYER NAME", newLayerName, 30);
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
      auto newName = std::string(newLayerName);
      if (!newName.empty()) {
        project->layerManager.addLayer(newName);
        // delete[] newLayerName;
        newLayerName = strdup("");
      }
    }
  }

  if (ImGui::CollapsingHeader("Movement Curves",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const auto &curve : project->GetAllMovementCurves()) {
      if (ImGui::Button((curve + "##curve").c_str()))
        project->OpenNewWindow(std::make_shared<MovementCurveEditingState>(
            curve, project->GetCurve(curve)));
    }
  }
  if (ImGui::Button("Create a new Curve")) {
    project->OpenNewWindow(std::make_shared<MovementCurveEditingState>());
  }

  if (ImGui::CollapsingHeader("Camera Options",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    const std::string splitOption =
        CAMERA_SPLIT_OPTIONS_REV[project->GetSplitCameraOption()];
    if (ImGui::BeginCombo("##split options combo", splitOption.c_str())) {
      for (auto entry : CAMERA_SPLIT_OPTIONS_REV) {
        SplitCameraOption splitEnum = entry.first;
        std::string splitString = entry.second;
        bool isSelected = splitEnum == project->GetSplitCameraOption();
        if (ImGui::Selectable(splitString.c_str(), isSelected)) {
          // manually set camera options so that we could save the previous
          // state when entering animation mode
          switch (splitEnum) {
          case UNSPLIT:
            project->Unsplit();
            break;
          case SPLITX:
            project->SplitX();
            break;
          case SPLITY:
            project->SplitY();
            break;
          }
          project->SetPrevSplitCameraOption(project->GetSplitCameraOption());
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::Text("Select Camera for screen 1:");
    const std::string currentCamera1 = project->GetCameraScreen1();
    if (ImGui::BeginCombo("##cameras combo 1", currentCamera1.c_str())) {
      for (auto cameraName : project->GetAllCameras()) {
        bool is_selected = (currentCamera1 == cameraName);
        if (ImGui::Selectable(cameraName.c_str(), is_selected)) {
          project->SetCameraScreen1(cameraName);
          std::cout << "Selected in camera screen 1 combo: " << currentCamera1
                    << std::endl;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    ImGui::Text("Select Camera for screen 2:");
    const std::string currentCamera2 = project->GetCameraScreen2();
    if (ImGui::BeginCombo("##cameras combo 2", currentCamera2.c_str())) {
      for (auto cameraName : project->GetAllCameras()) {
        bool is_selected = (currentCamera2 == cameraName);
        if (ImGui::Selectable(cameraName.c_str(), is_selected)) {
          project->SetCameraScreen2(cameraName);
          std::cout << "Selected in camera screen 2 combo: " << currentCamera2
                    << std::endl;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    ImGui::Text("Select Camera for animation:");
    const std::string currentCameraAnimation =
        project->GetCameraScreenAnimation();
    if (ImGui::BeginCombo("##cameras combo animation",
                          currentCameraAnimation.c_str())) {
      for (auto cameraName : project->GetAllCameras()) {
        bool is_selected = (currentCameraAnimation == cameraName);
        if (ImGui::Selectable(cameraName.c_str(), is_selected)) {
          if (project->GetCamera(cameraName)->mover == nullptr) {
            OpenErrorWindow(project, "Animation Camera Mover cannot be empty");
          } else {
            project->SetCameraScreenAnimation(cameraName);
            std::cout << "Selected in camera screen animation combo: "
                      << currentCameraAnimation << std::endl;
          }
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    if (ImGui::Button("Add Camera")) {
      project->guiStates.push(std::make_shared<CameraAddState>());
    }
  }

  if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
    switch (project->getAnimationStatus()) {
    case PLAYING:
      throw std::runtime_error(
          "Menu shouldn't be open on animation status PLAYING");
      break;
    case PAUSED:
      if (ImGui::Button("Play")) {
        if (project->GetCamera(project->GetCameraScreenAnimation()) == nullptr)
          OpenErrorWindow(project, "Cannot play without a main camera");
        else {
          project->OpenNewWindow(std::make_shared<MediaSliderState>());
          project->Play();
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        project->Stop();
      }
      ImGui::SameLine();
      if (ImGui::Button("Replay")) {
        project->OpenNewWindow(std::make_shared<MediaSliderState>());
        project->Replay();
      }
      break;
    case STOPPED:
      if (ImGui::Button("Play")) {
        if (project->GetCamera(project->GetCameraScreenAnimation()) == nullptr)
          OpenErrorWindow(project, "Cannot play without a main camera");
        else {
          project->OpenNewWindow(std::make_shared<MediaSliderState>());
          project->Play();
        }
      }
      break;
    }
    float time = (float)project->GetGlobalTime();
    float maxTime = project->maxTime();
    if (ImGui::SliderFloat("##Time Scale", &time, 0, maxTime, "%.1f")) {
      project->SetGlobalTime((long)time);
    }
  }

  if ((ImGui::IsKeyPressed(GLFW_KEY_RIGHT_CONTROL) ||
       ImGui::IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) &&
      ImGui::IsKeyPressed(GLFW_KEY_N))
    project->OpenNewWindow(std::make_shared<ShapeEditingState>());

  ImGui::PopFont();
}

MenuState::~MenuState() {
}

ErrorMsgState::ErrorMsgState(std::string errorMessage)
    : GuiState(ERROR), errorMessage(errorMessage) {}

void ErrorMsgState::Run(Project *project,
                        std::vector<igl::opengl::Camera *> &camera,
                        Eigen::Vector4i &viewWindow,
                        std::vector<DrawInfo *> drawInfos, ImFont *font,
                        ImFont *boldFont) {
  BeginCentered("Error");
  Header("Error", boldFont);
  ImGui::Text("%s", errorMessage.c_str());
  if (ButtonCenteredOnLine("Close"))
    project->CloseCurrentWindow();
}

ShapeEditingState::ShapeEditingState()
    : GuiState(SHAPE_EDITING), editingMode(CREATE_NEW), source(HARD_CODED),
      name(strdup("")), type(nullptr), shader(-1), sizePercents(100) {}

ShapeEditingState::ShapeEditingState(std::shared_ptr<SceneShape> shp,
                                     std::shared_ptr<SceneShader> scnShader)
    : GuiState(SHAPE_EDITING), editingMode(EDIT_EXISTING),
      source(shp->getSource()), name(strdup(shp->name.c_str())),
      file(shp->getFile()),
      type(std::make_shared<igl::opengl::glfw::Viewer::shapes>(shp->type)),
      layer(shp->getLayer()), shader(shp->shader),
      mover(shp->mover->cloneAndCast()), sizePercents(shp->GetScale()) {}

std::map<std::string, igl::opengl::glfw::Viewer::shapes> SHAPE_TYPES = {
    {"Axis", igl::opengl::glfw::Viewer::shapes::Axis},
    {"X Cylinder", igl::opengl::glfw::Viewer::shapes::xCylinder},
    {"Y Cylinder", igl::opengl::glfw::Viewer::shapes::yCylinder},
    {"Z Cylinder", igl::opengl::glfw::Viewer::shapes::zCylinder},
    {"Plane", igl::opengl::glfw::Viewer::shapes::Plane},
    {"Cube", igl::opengl::glfw::Viewer::shapes::Cube},
    {"Octahedron", igl::opengl::glfw::Viewer::shapes::Octahedron},
    {"Tetrahedron", igl::opengl::glfw::Viewer::shapes::Tethrahedron},
    {"LineCopy", igl::opengl::glfw::Viewer::shapes::LineCopy},
    {"MeshCopy", igl::opengl::glfw::Viewer::shapes::MeshCopy},
    {"Sphere", igl::opengl::glfw::Viewer::shapes::Sphere},
};

std::map<igl::opengl::glfw::Viewer::shapes, std::string> SHAPE_TYPES_REV = {
    {igl::opengl::glfw::Viewer::shapes::Axis, "Axis"},
    {igl::opengl::glfw::Viewer::shapes::xCylinder, "X Cylinder"},
    {igl::opengl::glfw::Viewer::shapes::yCylinder, "Y Cylinder"},
    {igl::opengl::glfw::Viewer::shapes::zCylinder, "Z Cylinder"},
    {igl::opengl::glfw::Viewer::shapes::Plane, "Plane"},
    {igl::opengl::glfw::Viewer::shapes::Cube, "Cube"},
    {igl::opengl::glfw::Viewer::shapes::Octahedron, "Octahedron"},
    {igl::opengl::glfw::Viewer::shapes::Tethrahedron, "Tetrahedron"},
    {igl::opengl::glfw::Viewer::shapes::LineCopy, "LineCopy"},
    {igl::opengl::glfw::Viewer::shapes::MeshCopy, "MeshCopy"},
    {igl::opengl::glfw::Viewer::shapes::Sphere, "Sphere"}};

void ShapeEditingState::Run(Project *project,
                            std::vector<igl::opengl::Camera *> &camera,
                            Eigen::Vector4i &viewWindow,
                            std::vector<DrawInfo *> drawInfos, ImFont *font,
                            ImFont *boldFont) {

  BeginCentered("Shape Editing");
  std::string header;
  if (editingMode == CREATE_NEW)
    header = "New shape";

  else
    header = "Edit: " + std::string(name);
  Header(header.c_str(), boldFont);
  if (ImGui::CollapsingHeader("Name", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (editingMode == CREATE_NEW)
      ImGui::InputText("##Name", name, 30);
    else
      ImGui::Text("%s", name);
  }
  if (editingMode == CREATE_NEW) {
    if (ImGui::CollapsingHeader("Source", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::RadioButton("From Menu", source == HARD_CODED)) {
        source = HARD_CODED;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("From File", source == FROM_FILE)) {
        source = FROM_FILE;
      }
    }
  }
  if (source == HARD_CODED) {
    if (ImGui::CollapsingHeader("Type", ImGuiTreeNodeFlags_DefaultOpen)) {
      std::string firstTypeName = "";
      if (type != nullptr)
        firstTypeName = SHAPE_TYPES_REV[*type];
      if (editingMode == EDIT_EXISTING)
        ImGui::Text("%s", firstTypeName.c_str());
      else {
        if (ImGui::BeginCombo("##Type combo", firstTypeName.c_str())) {
          for (int i = igl::opengl::glfw::Viewer::shapes::Axis;
               i != igl::opengl::glfw::Viewer::shapes::Sphere; i++) {
            igl::opengl::glfw::Viewer::shapes currentType =
                static_cast<igl::opengl::glfw::Viewer::shapes>(i);
            std::string typeName = SHAPE_TYPES_REV[currentType];
            bool isSelected = type != nullptr && currentType == *type;
            if (ImGui::Selectable(typeName.c_str(), isSelected)) {
              type = std::make_shared<igl::opengl::glfw::Viewer::shapes>(
                  currentType);
            }
            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
      }
    }
  }

  if (source == FROM_FILE) {
    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto allFiles = project->GetAllShapeFiles();
      if (editingMode == EDIT_EXISTING)
        ImGui::Text("%s", file.c_str());
      else {
        if (ImGui::BeginCombo("##File combo", file.c_str())) {
          for (auto shapeFile : allFiles) {
            bool isSelected = file == shapeFile;
            if (ImGui::Selectable(shapeFile.c_str(), isSelected)) {
              file = shapeFile;
            }
            if (isSelected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
      }
    }
  }
  if (ImGui::CollapsingHeader("Size"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::InputFloat("##Shape Size", &sizePercents);
  }

  if (ImGui::CollapsingHeader("Shader"), ImGuiTreeNodeFlags_DefaultOpen) {
    auto allShaders = project->GetAllShaders();
    std::string shaderName = project->GetShaderName(shader);
    if (ImGui::BeginCombo("##Shader combo", shaderName.c_str())) {
      for (auto &currentShader : allShaders) {
        bool isSelected = currentShader == shaderName;
        if (ImGui::Selectable(currentShader.c_str(), isSelected)) {
          shader = project->GetShaderId(currentShader);
          std::shared_ptr<SceneShader> scnShader = project->GetShader(shader);
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }

  if (ImGui::CollapsingHeader("Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto allLayers = project->layerManager.layers;
    std::string layerName;
    if (layer != nullptr)
      layerName = layer->getName();
    if (ImGui::BeginCombo("##Layer combo", layerName.c_str())) {
      for (auto currentLayerEntry : allLayers) {
        auto currentLayer = currentLayerEntry.second;
        bool isSelected = currentLayer == layer;
        if (ImGui::Selectable(currentLayer->getName().c_str(), isSelected)) {
          layer = currentLayer;
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }
  if (ImGui::CollapsingHeader("Movement Curves",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    auto allCurves = project->GetAllMovementCurves();
    std::string moverName;
    if (mover != nullptr)
      moverName = mover->name;
    if (ImGui::BeginCombo("##Mover combo", moverName.c_str())) {
      for (auto curve : allCurves) {
        bool isSelected = moverName == curve;
        if (ImGui::Selectable(curve.c_str(), isSelected)) {
          mover = project->GetCurve(curve);
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    bool saveSucceed = true;
    if (ImGui::Button("Save")) {
      if (std::string(name).empty()) {
        OpenErrorWindow(project, "Name cannot be empty!");
        saveSucceed = false;
      } else if (editingMode == CREATE_NEW &&
                 project->GetGlobalShape(name) != nullptr) {
        OpenErrorWindow(project,
                        "Shape: " + std::string(name) + " already exists!");
        saveSucceed = false;
      } else if (layer == nullptr) {
        OpenErrorWindow(project, "Layer cannot be empty!");
        saveSucceed = false;
      } else if (source == HARD_CODED && type == nullptr) {
        OpenErrorWindow(project, "Type cannot be empty!");
        saveSucceed = false;
      } else if (source == FROM_FILE && file.empty()) {
        OpenErrorWindow(project, "File cannot be empty!");
        saveSucceed = false;
      } else if (mover == nullptr) {
        OpenErrorWindow(project, "Curve cannot be empty!");
        saveSucceed = false;
      }
      if (editingMode == CREATE_NEW && saveSucceed) {
        std::shared_ptr<SceneShape> shape;
        if (source == HARD_CODED)
          shape = project->AddGlobalShape(name, *type, mover, layer,
                                          project->GetShaderName(shader));
        else
          shape = project->AddGlobalShape(name, file, mover, layer,
                                          project->GetShaderName(shader));
        shape->Rescale(sizePercents);
        project->CloseCurrentWindow();
      } else if (editingMode == EDIT_EXISTING && saveSucceed) {
        auto shape = project->GetGlobalShape(std::string(name));
        shape->name = name;
        shape->mover = mover;
        shape->shader = shader;
        if (layer != shape->getLayer()) {
          shape->getLayer()->deleteShape(shape);
          shape->changeLayer(layer);
          layer->addShape(shape);
        }

        if (sizePercents != shape->GetScale()) {
          shape->Rescale(sizePercents);
        }
        project->CloseCurrentWindow();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
      project->CloseCurrentWindow();
    }
  }
}

ShapeEditingState::~ShapeEditingState() {
  // delete[] name;
}

ShaderEditingState::ShaderEditingState(std::shared_ptr<SceneShader> scnShader)
    : GuiState(SHADER_EDITING), shaderName(scnShader->getName()) {
  for (const auto &param : scnShader->getParams())
    shaderParams.push_back(param->clone());
}

void ShaderEditingState::Run(Project *project,
                             std::vector<igl::opengl::Camera *> &camera,
                             Eigen::Vector4i &viewWindow,
                             std::vector<DrawInfo *> drawInfos, ImFont *font,
                             ImFont *boldFont) {
  BeginCentered("Editing shader");

  // filtering params for display ONLY
  std::vector<std::shared_ptr<ShaderParam>> paramsForDisplay;
  std::copy_if(shaderParams.begin(), shaderParams.end(),
               std::back_inserter(paramsForDisplay),
               // pred:
               [](const std::shared_ptr<ShaderParam> &param) {
                 return param->isForDisplay();
               });

  Header(("Editing shader: " + shaderName).c_str(), boldFont);
  if (paramsForDisplay.empty()) {
    ImGui::Text("This shader has no params to edit");
  } else {
    for (auto &param : shaderParams) {
      if (!param->isForDisplay()) {
        // nothing to display
        continue;
      } else if (param->getTag() == INT) {
        std::string label = param->getName() + " ##PARAM";
        ImGui::InputInt(
            label.c_str(),
            &std::dynamic_pointer_cast<ShaderIntParam>(param)->value);
      } else if (param->getTag() == FLOAT) {
        std::string label = param->getName() + " ##PARAM";
        ImGui::InputFloat(
            label.c_str(),
            &std::dynamic_pointer_cast<ShaderFloatParam>(param)->value);
      } else if (param->getTag() == VEC4_INT) {
        auto inputVec = std::dynamic_pointer_cast<ShaderIntVec4Param>(param);
        ImGui::Text("%s", (param->getName() + ":").c_str());
        ImGui::InputInt(("##PARAM" + param->getName() + "0").c_str(),
                        &inputVec->value[0]);
        ImGui::SameLine();
        ImGui::InputInt(("##PARAM" + param->getName() + "1").c_str(),
                        &inputVec->value[1]);
        ImGui::SameLine();
        ImGui::InputInt(("##PARAM" + param->getName() + "2").c_str(),
                        &inputVec->value[2]);
        ImGui::SameLine();
        ImGui::InputInt(("##PARAM" + param->getName() + "3").c_str(),
                        &inputVec->value[3]);
      } else if (param->getTag() == VEC4_FLOAT) {
        auto inputVec = std::dynamic_pointer_cast<ShaderFloatVec4Param>(param);
        ImGui::Text("%s", (param->getName() + ":").c_str());
        ImGui::InputFloat(("##PARAM" + param->getName() + "0").c_str(),
                          &inputVec->value[0]);
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "1").c_str(),
                          &inputVec->value[1]);
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "2").c_str(),
                          &inputVec->value[2]);
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "3").c_str(),
                          &inputVec->value[3]);
      } else if (param->getTag() == MAT4_FLOAT) {
        auto inputMat = std::dynamic_pointer_cast<ShaderFloatMat4Param>(param);
        ImGui::Text("%s", (param->getName() + ":").c_str());
        ImGui::InputFloat(("##PARAM" + param->getName() + "0,0").c_str(),
                          &inputMat->value(0));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "0,1").c_str(),
                          &inputMat->value(1));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "0,2").c_str(),
                          &inputMat->value(2));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "0,3").c_str(),
                          &inputMat->value(3));
        ImGui::InputFloat(("##PARAM" + param->getName() + "1,0").c_str(),
                          &inputMat->value(4));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "1,1").c_str(),
                          &inputMat->value(5));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "1,2").c_str(),
                          &inputMat->value(6));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "1,3").c_str(),
                          &inputMat->value(7));
        ImGui::InputFloat(("##PARAM" + param->getName() + "2,0").c_str(),
                          &inputMat->value(8));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "2,1").c_str(),
                          &inputMat->value(9));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "2,2").c_str(),
                          &inputMat->value(10));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "2,3").c_str(),
                          &inputMat->value(11));
        ImGui::InputFloat(("##PARAM" + param->getName() + "3,0").c_str(),
                          &inputMat->value(12));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "3,1").c_str(),
                          &inputMat->value(13));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "3,2").c_str(),
                          &inputMat->value(14));
        ImGui::SameLine();
        ImGui::InputFloat(("##PARAM" + param->getName() + "3,3").c_str(),
                          &inputMat->value(15));
      } else if (param->getTag() == RGB) {
        auto rgb = std::dynamic_pointer_cast<ShaderRGBParam>(param);
        ImGui::Text("%s", (param->getName() + ":").c_str());
        ImGui::ColorPicker4(("##" + param->getName() + "COLOR PICKING").c_str(),
                            rgb->value);
      } else if (param->getTag() == TRANSPARENCY) {
        std::string label = param->getName() + "##PARAM";
        ImGui::SliderFloat(
            label.c_str(),
            &std::dynamic_pointer_cast<ShaderTransparencyParam>(param)->value,
            0, 100, "%.1f");
      }
    }
  }
  if (!paramsForDisplay.empty()) {
    if (ImGui::Button("Save")) {
      project->GetShader(shaderName)->setParams(shaderParams);
      project->CloseCurrentWindow();
    }
    ImGui::SameLine();
  }
  if (ImGui::Button("Close")) {
    project->CloseCurrentWindow();
  }
}

CameraAddState::CameraAddState()
    : GuiState(CAMERA_ADD), position(new float[3]{0, 0, 0}) {}

//
//    std::string name = "Camera" + project->GetAllCameras().size();
//    float angle = 0,relationWH = 1, near = 1, far = 120;

void CameraAddState::Run(Project *project,
                         std::vector<igl::opengl::Camera *> &camera,
                         Eigen::Vector4i &viewWindow,
                         std::vector<DrawInfo *> drawInfos, ImFont *font,
                         ImFont *boldFont) {
  BeginCentered("Camera Add");
  std::string header;
  // not sure why it's needed
  if (editingMode == CREATE_NEW) {
    header = "Add Camera";
    Header(header.c_str(), boldFont);
  }

  ImGui::Text("Name: ");
  ImGui::SameLine();
  ImGui::InputText("##Name", cameraName, 30);
  ImGui::Text("Position (x,y,z)");
  ImGui::InputFloat3("##Camera Position", position);

  //    ImGui::Text("Angle: ");
  //    ImGui::SameLine();
  //    ImGui::InputFloat("##angle", &angle);
  //
  //    ImGui::Text("RelationWH: ");
  //    ImGui::SameLine();
  //    ImGui::InputFloat("##relationWH", &relationWH);

  //    ImGui::Text("Near: ");
  //    ImGui::SameLine();
  //    ImGui::InputFloat("##near", &near);
  //
  //    ImGui::Text("Far: ");
  //    ImGui::SameLine();
  //    ImGui::InputFloat("##far", &far);

  if (ImGui::CollapsingHeader("Movement Curves",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    auto allCurves = project->GetAllMovementCurves();
    std::string moverName;
    if (mover != nullptr)
      moverName = mover->name;
    if (ImGui::BeginCombo("##Mover combo", moverName.c_str())) {
      for (auto curve : allCurves) {
        bool isSelected = moverName == curve;
        if (ImGui::Selectable(curve.c_str(), isSelected)) {
          mover = project->GetCurve(curve);
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }

  if (ImGui::Button("Save ")) {
    bool saveSuccess = true;
    if (std::string(cameraName).empty()) {
      OpenErrorWindow(project, "Name cannot be empty!");
      saveSuccess = false;
    }
    if (saveSuccess) {
      project->AddGlobalCamera(
          cameraName, Eigen::Vector3d(position[0], position[1], position[2]),
          mover);
      project->guiStates.pop();
    }
  }
  ImGui::SameLine();

  if (ImGui::Button("Close")) {
    project->guiStates.pop();
  }
}

MovementCurveEditingState::MovementCurveEditingState(
    std::string name, std::shared_ptr<ObjectMoverSplit> mover)
    : GuiState(CURVE_EDITING), curveName(strdup(name.c_str())),
      editingMode(EDIT_EXISTING), startTime(mover->getStartTime()) {
  for (auto &subMover : mover->movers)
    movers.push_back(std::make_shared<ObjectMoverForGui>(subMover));
}

MovementCurveEditingState::MovementCurveEditingState()
    : GuiState(CURVE_EDITING), editingMode(CREATE_NEW), startTime(0),
      curveName(strdup("")) {}

void MovementCurveEditingState::Run(Project *project,
                                    std::vector<igl::opengl::Camera *> &camera,
                                    Eigen::Vector4i &viewWindow,
                                    std::vector<DrawInfo *> drawInfos,
                                    ImFont *font, ImFont *boldFont) {
  BeginCentered("Editing Curve");
  if (editingMode == CREATE_NEW) {
    Header("New Curve", boldFont);
  } else {
    Header(("Editing: " + std::string(curveName)).c_str(), boldFont);
  }
  ImGui::Text("Name: ");
  ImGui::SameLine();
  if (editingMode == CREATE_NEW) {
    ImGui::InputText("##NAME", curveName, 30);
  } else {
    ImGui::Text("%s", curveName);
  }
  ImGui::Text("Start Time: ");
  ImGui::SameLine();
  ImGui::InputFloat("##start time of curve", &startTime);
  for (size_t i = 0; i < movers.size(); i++) {
    auto mover = movers[i];
    if (ImGui::Button(("Delete##" + std::string(curveName) + std::to_string(i))
                          .c_str())) {
      movers.erase(movers.begin() + i);
      i--;
      continue;
    }
    ImGui::Text("Duration: ");
    ImGui::SameLine();
    ImGui::InputFloat(("##duration of mover" + std::to_string(i)).c_str(),
                      &mover->duration);
    if (ImGui::RadioButton(
            ("Constant##" + std::string(curveName) + std::to_string(i)).c_str(),
            mover->moverType == CONSTANT)) {
      mover->moverType = CONSTANT;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(
            ("Bezier##" + std::string(curveName) + std::to_string(i)).c_str(),
            mover->moverType == BEZIER)) {
      mover->moverType = BEZIER;
    }
    std::vector<float *> *points = &mover->points;
    for (size_t j = 0; j < points->size(); j++) {
      float *point = (*points)[j];
      if (ImGui::Button(("X##Delete" + std::string(curveName) +
                         std::to_string(i) + "Point" + std::to_string(j))
                            .c_str())) {
        delete[] point;
        points->erase(points->begin() + j);
        j--;
        continue;
      }
      ImGui::SameLine();
      ImGui::InputFloat3(("##Input" + std::string(curveName) +
                          std::to_string(i) + "Point" + std::to_string(j))
                             .c_str(),
                         point);
    }
    if (ImGui::Button(("Add Point##" + std::to_string(i)).c_str()))
      mover->points.push_back(new float[3]{0, 0, 0});
  }
  if (ImGui::Button("Add Mover")) {
    movers.push_back(std::make_shared<ObjectMoverForGui>());
  }
  if (ButtonCenteredOnLine("Save")) {
    bool saveSucceed = true;
    std::vector<std::shared_ptr<ObjectMover>> objectsForSave;
    auto nextStartTime = startTime;
    for (auto m : movers) {
      if (m->points.size() == 0) {
        OpenErrorWindow(project, "Movers must have 1 points at least");
        saveSucceed = false;
        break;
      }
      if (m->moverType == CONSTANT && m->points.size() > 1) {
        OpenErrorWindow(project, "Constant movers may have only one point");
        saveSucceed = false;
        break;
      }

      std::shared_ptr<ObjectMover> savedMover;
      if (m->moverType == CONSTANT) {
        savedMover = std::make_shared<ObjectMoverConstant>(
            Eigen::Vector3f(m->points[0][0], m->points[0][1], m->points[0][2]),
            nextStartTime, m->duration);
      } else {
        std::vector<Eigen::Vector3f> bezPoints;
        for (auto p : m->points) {
          bezPoints.push_back(Eigen::Vector3f(p[0], p[1], p[2]));
        }
        savedMover = std::make_shared<ObjectMoverBezier>(
            bezPoints, nextStartTime, m->duration);
      }
      objectsForSave.push_back(savedMover);
      nextStartTime += m->duration;
    }
    if (saveSucceed) {
      if (editingMode == CREATE_NEW) {
        if (!project->AddMovementCurve(
                std::make_shared<ObjectMoverSplit>(objectsForSave, curveName)))
          OpenErrorWindow(project,
                          "Curve: " + std::string(curveName) + " exists");
      } else {
        project->SetNewMoversForCurve(std::string(curveName), objectsForSave);
      }
      project->CloseCurrentWindow();
    }
  }
  ImGui::SameLine();
  if (ButtonCenteredOnLine("Close")) {
    project->CloseCurrentWindow();
  }
}

MediaSliderState::MediaSliderState() : GuiState(MEDIA_SLIDER) {}

void MediaSliderState::Run(Project *project,
                           std::vector<igl::opengl::Camera *> &camera,
                           Eigen::Vector4i &viewWindow,
                           std::vector<DrawInfo *> drawInfos, ImFont *font,
                           ImFont *boldFont) {
  BeginSliderWindow("Slider");
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + io.DisplaySize.x * 0.352);
  switch (project->getAnimationStatus()) {
  case PLAYING:
    if (ImGui::Button("Pause")) {
      project->CloseCurrentWindow();
      project->Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      // restore version of previous state
      project->CloseCurrentWindow();
      project->Stop();
    }
    ImGui::SameLine();
    if (ImGui::Button("Replay")) {
      project->Replay();
    }
    break;
  case PAUSED:
  case STOPPED:
    project->CloseCurrentWindow();
    break;
  }
  float time = (float)project->GetGlobalTime();
  float maxTime = project->maxTime();
  ImGui::SetNextItemWidth(io.DisplaySize.x * 0.8);
  if (ImGui::SliderFloat("##Time Scale", &time, 0, maxTime, "%.1f")) {
    project->SetGlobalTime((long)time);
  }
}
