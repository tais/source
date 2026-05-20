# Multiplayer is deferred indefinitely (see docs/SDL3_PORT.md). The
# only RakNet we have is a prebuilt 32-bit Win32 RakNetLibStatic.lib
# (RakNet 3.401, no public source), which can't link into the 64-bit
# builds we ship. Until the wrapper is ported to a modern netlib,
# build the no-op stub everywhere: mp_stubs.cpp gives do-nothing
# definitions for every exported Multiplayer symbol so the main
# executable links and the game runs single-player.
#
# STATIC: the lib references main-executable globals (gTacticalStatus,
# ScreenMsg, ...); a Windows DLL can't leave those unresolved, and
# SDL3_mixer otherwise flips BUILD_SHARED_LIBS on globally. Static
# folds it into the executable.
add_library(Multiplayer STATIC
  "mp_stubs.cpp"
)

target_include_directories(Multiplayer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  "raknet"
)
