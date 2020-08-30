cmake_minimum_required(VERSION 2.8)

project(lua)

set(lua_library
  lapi.c
  lauxlib.c
  lbaselib.c
  lcode.c
  lcorolib.c
  lctype.c
  ldblib.c
  ldebug.c
  ldo.c
  ldump.c
  lfunc.c
  lgc.c
  linit.c
  liolib.c
  llex.c
  lmathlib.c
  lmem.c
  loadlib.c
  lobject.c
  lopcodes.c
  loslib.c
  lparser.c
  lstate.c
  lstring.c
  lstrlib.c
  ltable.c
  ltablib.c
  ltm.c
  lua.c
  luac.c
  lundump.c
  lutf8lib.c
  lvm.c
  lzio.c
)

foreach(s ${lua_library})
    set(lua_LIBRARY ${lua_LIBRARY} src/${s})
endforeach()

add_definitions(-DLUA_ANSI=1)
add_library(lua STATIC ${lua_LIBRARY})
