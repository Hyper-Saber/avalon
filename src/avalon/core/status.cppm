export module avalon.core:status;

export namespace avalon {
enum class EStatusCode {
  WindowError,
  SymbolNotFound,
  PluginInitializeError,
  FileNotFound,
  RhiError,
  OutOfMemory,
  DeviceLost,
  RhiUpdateFailed,
  InvalidParameter,
  NotSupported,
  NotFound,
  InternalError,
};
}
