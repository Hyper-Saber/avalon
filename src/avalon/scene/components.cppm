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
  float farPlane = 1000.f;

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

struct alignas(16) LightComponent {
  Vec4 color = Vec4(1.f, 1.f, 1.f, 1.f);
  Vec4 directionOrPosition{0.f, -1.f, 0.f, 10.f};
  scene::ELightType lightType = scene::ELightType::Directional;
  float padding[3];
};

} // namespace avalon::ecs
