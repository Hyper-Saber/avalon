module;
export module avalon.scene:components;

import :types;
import avalon.core;

export namespace avalon::ecs {

struct AVALON_SCENE_API CameraComponent {
  scene::EProjectionType projectionType = scene::EProjectionType::Perspective;

  float fov = 60.f;
  float aspectRatio = 16.f / 9.f;
  float nearPlane = 0.1f;
  float farPlane = 10.f;

  bool isDirty = true;
  Matrix4x4 projectionMatrix;

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

  void setFarPlane(float farPlane) {
    this->farPlane = farPlane;
    isDirty = true;
  }

  void UpdateProjectionMatrix() {
    if (!isDirty)
      return;

    projectionMatrix =
        CalculatePerspectiveMatrix(fov, aspectRatio, nearPlane, farPlane);
    isDirty = false;
  }
};

struct LightComponent {
  scene::ELightType type = scene::ELightType::Directional;
  Color color = Color(1.f, 1.f, 1.f, 1.f);
  float intensity = 1.f;
  float range = 10.f;
  float spotInnerRadians = 1.f;
  float spotOuterRadians = 2.f;
};

} // namespace avalon::ecs
