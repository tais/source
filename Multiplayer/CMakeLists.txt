if(WIN32)
  add_library(Multiplayer
    "client.cpp"
    "server.cpp"
    "transfer_rules.cpp"
  )
  target_link_libraries(Multiplayer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/raknet/RakNetLibStatic.lib"
  )
else()
  # RakNet 3.401 source isn't publicly available; the prebuilt .lib
  # is Win32-only. mp_stubs.cpp provides no-op definitions for every
  # exported symbol so the main executable still links.
  add_library(Multiplayer
    "mp_stubs.cpp"
  )
endif()

target_include_directories(Multiplayer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  "raknet"
)
