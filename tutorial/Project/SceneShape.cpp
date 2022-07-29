#include "SceneShape.h"

SceneShape::SceneShape(std::string shapeName,
                       igl::opengl::glfw::Viewer::shapes shapeType,
                       std::shared_ptr<ObjectMoverSplit> moverr,
                       std::shared_ptr<Layer> layer, int *indexes)
    : name(shapeName), type(shapeType), layer(layer),
      dimensions(Eigen::Vector3f(1, 1, 1)), isScaledToZero(false),
      mover(moverr), source(HARD_CODED), sizePercent(100), previousSize(100),
      isSelected(false) {
  for (int i = 0; i < 4; i++) {
    lastDrawnPositions[i] = Eigen::Vector3f(0, 0, 0);
    ids[i] = indexes[i];
    idsToRescale.push_back(indexes[i]);
  }
}
SceneShape::SceneShape(std::string shapeName, std::string file,
                       std::shared_ptr<ObjectMoverSplit> moverr,
                       std::shared_ptr<Layer> layer, int *indexes)
    : name(shapeName), file(file), layer(layer),
      dimensions(Eigen::Vector3f(1, 1, 1)), isScaledToZero(false),
      mover(moverr), source(FROM_FILE), sizePercent(100), previousSize(100),
      isSelected(false) {
  for (int i = 0; i < 4; i++) {
    lastDrawnPositions[i] = Eigen::Vector3f(0, 0, 0);
    ids[i] = indexes[i];
    idsToRescale.push_back(indexes[i]);
  }
}
std::shared_ptr<Layer> SceneShape::getLayer() { return layer; }

void SceneShape::changeLayer(std::shared_ptr<Layer> layer) {
  this->layer = layer;
}

int SceneShape::getIndex() { return index; }

bool SceneShape::isDrawn(float time) {
  return mover->isDrawnAt(time) && !layer->isHidden();
}

Eigen::Vector3f SceneShape::getPosition(float time) {
  return mover->getPosition(time);
}

Eigen::Vector3f SceneShape::getlastDrawnPosition(int id) {
  for (int i = 0; i < 4; i++) {
    if (id == ids[i]) {
      return lastDrawnPositions[i];
    }
  }
  return Eigen::Vector3f(0.0, 0.0, 0.0);
}

void SceneShape::setlastDrawnPosition(Eigen::Vector3f pos, int id) {
  for (int i = 0; i < 4; i++) {
    if (id == ids[i]) {
      lastDrawnPositions[i] = pos;
      break;
    }
  }
}

void SceneShape::addMover(std::shared_ptr<ObjectMover> mover) {
  this->mover->addMover(mover);
}

int SceneShape::getParent() { return parent; }

void SceneShape::setParent(int newParent) { parent = newParent; }

std::vector<int> SceneShape::getChildren() { return children; }

void SceneShape::addChild(int child) { children.push_back(child); }

void SceneShape::removeChild(int removedChild) {
  children.erase(std::remove(children.begin(), children.end(), removedChild));
}

float SceneShape::getEndTime() { return mover->getEndTime(); }

float SceneShape::GetNormalizedScale() {
  return ((float)sizePercent) / previousSize;
}

float SceneShape::GetScale() { return sizePercent; }

void SceneShape::Rescale(float newSize) {
  previousSize = sizePercent;
  sizePercent = newSize;
  idsToRescale = std::vector<int>(std::begin(ids), std::end(ids));
}

bool SceneShape::NeedsRescale(int id) {
  return std::find(idsToRescale.begin(), idsToRescale.end(), id) !=
         idsToRescale.end();
}

void SceneShape::MarkAsRescaled(int id) {
  std::vector<int> newIdsToRescale;
  std::copy_if(idsToRescale.begin(), idsToRescale.end(),
               std::back_inserter(newIdsToRescale),
               [id](int shapeId) { return shapeId != id; });
  idsToRescale = newIdsToRescale;
}

std::vector<int> SceneShape::GetIds() {
  return std::vector<int>(std::begin(ids), std::end(ids));
}
