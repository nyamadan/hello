cmake_minimum_required(VERSION 3.0.0)
project(lua VERSION 5.4.4)

include(GNUInstallDirs)

set(LUA_SOURCES
  ./src/lapi.c
  ./src/lcode.c
  ./src/lctype.c
  ./src/ldebug.c
  ./src/ldo.c
  ./src/ldump.c
  ./src/lfunc.c
  ./src/lgc.c
  ./src/llex.c
  ./src/lmem.c
  ./src/lobject.c
  ./src/lopcodes.c
  ./src/lparser.c
  ./src/lstate.c
  ./src/lstring.c
  ./src/ltable.c
  ./src/ltm.c
  ./src/lundump.c
  ./src/lvm.c
  ./src/lzio.c
  ./src/lauxlib.c
  ./src/lbaselib.c
  ./src/ldblib.c
  ./src/liolib.c
  ./src/lmathlib.c
  ./src/loslib.c
  ./src/ltablib.c
  ./src/lstrlib.c
  ./src/lutf8lib.c
  ./src/loadlib.c
  ./src/lcorolib.c
  ./src/linit.c
)

add_library(lua SHARED ${LUA_SOURCES})

if(MSVC OR MINGW)
  target_compile_definitions(lua PUBLIC LUA_BUILD_AS_DLL)
endif()

if(MINGW)
  target_link_options(lua PRIVATE "-static" "-lstdc++")
endif()

set_target_properties(lua PROPERTIES DEBUG_POSTFIX "d")

install(TARGETS lua EXPORT lua-config)

set_property(TARGET lua PROPERTY PUBLIC_HEADER
  ./src/lapi.h
  ./src/lauxlib.h
  ./src/lcode.h
  ./src/lctype.h
  ./src/ldebug.h
  ./src/ldo.h
  ./src/lfunc.h
  ./src/lgc.h
  ./src/ljumptab.h
  ./src/llex.h
  ./src/llimits.h
  ./src/lmem.h
  ./src/lobject.h
  ./src/lopcodes.h
  ./src/lopnames.h
  ./src/lparser.h
  ./src/lprefix.h
  ./src/lstate.h
  ./src/lstring.h
  ./src/ltable.h
  ./src/ltm.h
  ./src/lua.h
  ./src/luaconf.h
  ./src/lualib.h
  ./src/lundump.h
  ./src/lvm.h
  ./src/lzio.h
)

install(TARGETS lua
  EXPORT lua-config
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)