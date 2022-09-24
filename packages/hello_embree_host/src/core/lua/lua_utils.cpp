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

const char *FUNCTION_KEY = "93202c1a-90c3-4248-a174-5a37033be3c8";

void setFunctionTableToRegistry(lua_State *L) {
  lua_pushstring(L, FUNCTION_KEY);
  lua_newtable(L);
  lua_settable(L, LUA_REGISTRYINDEX);
}

int L_registerFunction(lua_State *L) {
  auto name = luaL_checkstring(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  lua_pushstring(L, FUNCTION_KEY);
  if (lua_gettable(L, LUA_REGISTRYINDEX) != LUA_TTABLE) {
    setFunctionTableToRegistry(L);
    lua_pushstring(L, FUNCTION_KEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
  }

  lua_pushstring(L, name);
  lua_pushvalue(L, 2);
  lua_settable(L, -3);

  return 0;
}

int L_unregisterFunction(lua_State *L) {
  auto name = luaL_checkstring(L, 1);

  lua_pushstring(L, FUNCTION_KEY);
  if (lua_gettable(L, LUA_REGISTRYINDEX) != LUA_TTABLE) {
    setFunctionTableToRegistry(L);
    lua_pushstring(L, FUNCTION_KEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
  }

  lua_pushstring(L, name);
  lua_pushnil(L);
  lua_settable(L, -3);
  return 0;
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

  lua_pushcfunction(L, L_registerFunction);
  lua_setfield(L, -2, "registerFunction");

  lua_pushcfunction(L, L_unregisterFunction);
  lua_setfield(L, -2, "unregisterFunction");

  return 1;
}

} // namespace

namespace hello::lua::utils {
void openlibs(lua_State *L) {
  luaL_requiref(L, "utils", L_require, false);
  lua_pop(L, 1);
}

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

int getFunction(lua_State *L, const char *const name) {
  lua_pushstring(L, FUNCTION_KEY);
  if (lua_gettable(L, LUA_REGISTRYINDEX) != LUA_TTABLE) {
    lua_pop(L, 1);
    setFunctionTableToRegistry(L);
    lua_pushstring(L, FUNCTION_KEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
  }

  lua_pushstring(L, name);
  lua_gettable(L, -2);
  lua_remove(L, -2);

  return lua_type(L, -1);
}

} // namespace hello::lua::utils
