module;
export module avalon.core:status;

export namespace avalon {
enum class EStatusCode {
  Success,
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
  InternalError,
};
}
