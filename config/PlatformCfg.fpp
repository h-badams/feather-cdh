# =======================================================================
# FPP file for configuration of various F Prime Platform and Os constants.
# Project-level override of lib/fprime/default/config/PlatformCfg.fpp.
# Registered via CONFIGURATION_OVERRIDES in the root CMakeLists.txt.
#
# FW_FILE_HANDLE_MAX_SIZE and FW_DIRECTORY_HANDLE_MAX_SIZE are raised to 64
# to accommodate ArduinoFileHandle / ArduinoDirectoryHandle (~40 bytes each)
# used by the fprime-arduino ARM platform. 64 bytes is safe for all targets
# (native Linux handles need ≤16 bytes; larger storage is always harmless).
# =======================================================================

@ Maximum size of a handle for Os::Console
constant FW_CONSOLE_HANDLE_MAX_SIZE = 24

@ Maximum size of a handle for Os::Task
constant FW_TASK_HANDLE_MAX_SIZE = 40

@ Maximum size of a handle for Os::File
@ Raised to 64 for Arduino SD::File compatibility (default is 16)
constant FW_FILE_HANDLE_MAX_SIZE = 64

@ Maximum size of a handle for Os::Mutex
constant FW_MUTEX_HANDLE_MAX_SIZE = 72

@ Maximum size of a handle for Os::Queue
constant FW_QUEUE_HANDLE_MAX_SIZE = 368

@ Maximum size of a handle for Os::Directory
@ Raised to 64 for Arduino SD::File compatibility (default is 16)
constant FW_DIRECTORY_HANDLE_MAX_SIZE = 64

@ Maximum size of a handle for Os::FileSystem
constant FW_FILESYSTEM_HANDLE_MAX_SIZE = 16

@ Maximum size of a handle for Os::RawTime
constant FW_RAW_TIME_HANDLE_MAX_SIZE = 56

@ Maximum allowed serialization size for Os::RawTime objects
constant FW_RAW_TIME_SERIALIZATION_MAX_SIZE = 8

@ Maximum size of a handle for Os::ConditionVariable
constant FW_CONDITION_VARIABLE_HANDLE_MAX_SIZE = 56

@ Maximum size of a handle for Os::Cpu
constant FW_CPU_HANDLE_MAX_SIZE = 16

@ Maximum size of a handle for Os::Memory
constant FW_MEMORY_HANDLE_MAX_SIZE = 16

@ Alignment of handle storage
constant FW_HANDLE_ALIGNMENT = 8

@ Chunk size for working with files in the OSAL layer
constant FW_FILE_CHUNK_SIZE = 512
