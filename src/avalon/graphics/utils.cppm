module;
export module avalon.graphics:utils;

import avalon.core;
import :mesh_manager;
import :material_manager;

export namespace avalon::graphics {

inline auto GetMeshManager() -> MeshManager & {
  return GetContext().GetService<MeshManager>(EEngineService::MeshManager);
}

inline auto GetMaterialManager() -> MaterialManager & {
  return GetContext().GetService<MaterialManager>(
      EEngineService::MaterialManager);
}

inline ProbeData CreateSkyboxProbeData() {
  ProbeData probe;

  struct FaceOrientation {
    Vec3 forward;
    Vec3 up;
  };

  const FaceOrientation kFaceOrientations[6] = {
      {Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)},  // +X (Right)
      {Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)}, // -X (Left)
      {Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)},   // +Y (Up/Top)
      {Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f)}, // -Y (Down/Bottom)
      {Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f)},  // +Z (Back)
      {Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f)}  // -Z (Forward)
  };

  for (int i = 0; i < 6; ++i) {
    Quaternion q = Quaternion::LookRotation(kFaceOrientations[i].forward,
                                            kFaceOrientations[i].up);

    probe.captureViews[i] = q.ToMatrix();
  }

  return probe;
}

} // namespace avalon::graphics
