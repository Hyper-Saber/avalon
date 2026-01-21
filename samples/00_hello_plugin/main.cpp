import avalon.core;

int main() {
  const char *pluginPath = "libavalon.mock.plugin.so";
  avalon::Info("Attempting to load plugin...");

  auto result = avalon::LoadPlugin<avalon::IPlugin>(pluginPath);
  if (result) {
    avalon::Info("Plugin loaded successfully!");

  } else {
    avalon::Error("Failed to load plugin! Error code: {}.",
                  (int)result.error());
  }

  return 0;
}
