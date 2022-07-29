#include "ProjectViewerData.h"

IGL_INLINE ProjectViewerData::ProjectViewerData()
    : igl::opengl::ViewerData::ViewerData(), camera_split(0),
      is_visible(~unsigned(0)) {
  cameras.emplace_back("Main Camera");
}

void ProjectViewerData::set_camera_split(int _camera_split_option) {
  camera_split = _camera_split_option;
}

void ProjectViewerData::set_visible(bool value, unsigned int property_id) {
  if (value)
    is_visible |= property_id;
  else
    is_visible &= ~property_id;
}

void ProjectViewerData::clear() {
  ViewerData::clear();
  face_based = false;
}
