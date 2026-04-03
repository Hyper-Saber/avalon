module;
export module avalon.scene:camera_component;

import :types;
import avalon.core;

export namespace avalon::ecs {

struct AVALON_SCENE_API CameraComponent {
  scene::EProjectionType projectionType = scene::EProjectionType::Perspective;

  float fov = 75.f;
  float aspectRatio = 16.f / 9.f;
  float nearPlane = 0.1f;

  bool isDirty = true;
  Matrix4x4 projectionMatrix;
  Matrix4x4 inverseProjectionMatrix;

  float sensitivity = 1.5f;
  Vec3 currentRotation = {0, 0, 0};

  void SetFov(float fov) {
    this->fov = fov;
    isDirty = true;
  }

  void SetAspectRatio(float aspectRatio) {
    this->aspectRatio = aspectRatio;
    isDirty = true;
  }

  void SetNearPlane(float nearPlane) {
    this->nearPlane = nearPlane;
    isDirty = true;
  }

  void UpdateProjectionMatrix() {
    if (!isDirty)
      return;

    projectionMatrix = CalculatePerspectiveMatrix(fov, aspectRatio, nearPlane);
    inverseProjectionMatrix =
        CalculateInversePerspective(fov, aspectRatio, nearPlane);
    isDirty = false;
  }
};

} // namespace avalon::ecs
