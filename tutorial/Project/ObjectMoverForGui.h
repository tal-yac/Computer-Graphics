#pragma once

#include "ObjectMover.h"
#include <memory>

class ObjectMoverForGui {
public:
  ObjectMoverForGui(std::shared_ptr<ObjectMover> mover);
  ObjectMoverForGui();
  std::vector<float *> points;
  MoverType moverType;
  float duration;
  ~ObjectMoverForGui();
};