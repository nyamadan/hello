#include "lua_utils.hpp"

#include <iostream>
#include <map>
#include <string>

namespace {
int msghandler(lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {                         /* is error object not a string? */
    if (luaL_callmeta(L, 1, "__tostring") && /* does it have a metamethod */
        lua_type(L, -1) == LUA_TSTRING)      /* that produces a string? */
      return 1;                              /* that is the message */
    else
      msg = lua_pushfstring(L, "(error object is a %s value)",
                            luaL_typename(L, 1));
  }
  luaL_traceback(L, L, msg, 1); /* append a standard traceback */
  return 1;                     /* return the traceback */
}

int L_isEmscripten(lua_State *L) {
#if defined(__EMSCRIPTEN__)
  lua_pushboolean(L, 1);
#else
  lua_pushboolean(L, 0);
#endif
  return 1;
}

int L_isApple(lua_State *L) {
#if defined(__APPLE__)
  lua_pushboolean(L, 1);
#else
  lua_pushboolean(L, 0);
#endif
  return 1;
}

int L_isWindows(lua_State *L) {
#if defined(__MINGW32__) || defined(_MSC_VER)
  lua_pushboolean(L, 1);
#else
  lua_pushboolean(L, 0);
#endif
  return 1;
}

int L_isLinux(lua_State *L) {
#if defined(__linux__)
  lua_pushboolean(L, 1);
#else
  lua_pushboolean(L, 0);
#endif
  return 1;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushcfunction(L, L_isEmscripten);
  lua_setfield(L, -2, "isEmscripten");

  lua_pushcfunction(L, L_isWindows);
  lua_setfield(L, -2, "isWindows");

  lua_pushcfunction(L, L_isApple);
  lua_setfield(L, -2, "isApple");

  lua_pushcfunction(L, L_isLinux);
  lua_setfield(L, -2, "isLinux");

  return 1;
}

} // namespace

namespace hello::lua {
void openlibs(lua_State *L) { luaL_requiref(L, "lua", L_require, false); }

int docall(lua_State *L, int narg, int nres) {
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, msghandler); /* push message handler */
  lua_insert(L, base);              /* put it under function and args */
  status = lua_pcall(L, narg, nres, base);
  lua_remove(L, base); /* remove message handler from the stack */
  return status;
}

int dostring(lua_State *L, const char *const s, int narg, int nres) {
  auto r1 = luaL_loadstring(L, s);
  if (r1 != LUA_OK) {
    return r1;
  }
  return docall(L, narg, nres);
}

int report(lua_State *L, int status) {
  if (status != LUA_OK) {
    auto msg = lua_tostring(L, -1);
    auto top = lua_gettop(L);
    lua_writestringerror("%s\n", msg);
    lua_settop(L, top - 1); /* remove message */
  }
  return status;
}
} // namespace hello::lua
