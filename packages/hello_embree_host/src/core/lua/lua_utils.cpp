#include "lua_utils.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

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

#if defined(__EMSCRIPTEN__)
const char *const FETCH_REQUEST_NAME = "FetchRequest";

struct FetchRequest {
  emscripten_fetch_t *data = nullptr;
  bool finished = false;
  bool succeeded = false;
};

void downloadSucceeded(emscripten_fetch_t *fetch) {
  auto udFetchRequest = static_cast<FetchRequest *>(fetch->userData);
  udFetchRequest->finished = true;
  udFetchRequest->succeeded = true;
}

void downloadFailed(emscripten_fetch_t *fetch) {
  auto udFetchRequest = static_cast<FetchRequest *>(fetch->userData);
  udFetchRequest->finished = true;
  udFetchRequest->succeeded = false;
}

int L_getFetchRequest(lua_State *L) {
  auto udFetchRequest =
      static_cast<FetchRequest *>(luaL_checkudata(L, 1, FETCH_REQUEST_NAME));
  luaL_argcheck(L, udFetchRequest->data != nullptr, 1,
                "FetchRequest->data is null.");

  auto fetch = udFetchRequest->data;
  lua_newtable(L);
  lua_pushinteger(L, fetch->id);
  lua_setfield(L, -2, "id");
  lua_pushstring(L, fetch->url);
  lua_setfield(L, -2, "url");
  if (fetch->numBytes > 0 && udFetchRequest->finished) {
    auto udBuffer = hello::lua::buffer::alloc(L, fetch->numBytes);
    memcpy(udBuffer->data, fetch->data, fetch->numBytes);
    lua_setfield(L, -2, "data");
  } else {
    lua_pushnil(L);
    lua_setfield(L, -2, "data");
  }
  lua_pushinteger(L, fetch->dataOffset);
  lua_setfield(L, -2, "dataOffset");
  lua_pushinteger(L, fetch->totalBytes);
  lua_setfield(L, -2, "totalBytes");
  lua_pushinteger(L, fetch->readyState);
  lua_setfield(L, -2, "readyState");
  lua_pushinteger(L, fetch->status);
  lua_setfield(L, -2, "status");
  lua_pushstring(L, fetch->statusText);
  lua_setfield(L, -2, "statusText");
  lua_pushboolean(L, udFetchRequest->finished);
  lua_setfield(L, -2, "finished");
  lua_pushboolean(L, udFetchRequest->succeeded);
  lua_setfield(L, -2, "succeeded");
  return 1;
}

int L_freeFetchRequest(lua_State *L) {
  auto udFetchRequest =
      static_cast<FetchRequest *>(luaL_checkudata(L, 1, FETCH_REQUEST_NAME));
  if (udFetchRequest->data != nullptr) {
    udFetchRequest->finished = false;
    udFetchRequest->succeeded = false;
    udFetchRequest->data->userData = nullptr;
    emscripten_fetch_close(udFetchRequest->data);
    udFetchRequest->data = nullptr;
  }
  return 0;
}

int L_fetch(lua_State *L) {
  auto url = luaL_checkstring(L, 1);
  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = downloadSucceeded;
  attr.onerror = downloadFailed;
  auto fetch = emscripten_fetch(&attr, url);
  auto udFetchRequest =
      static_cast<FetchRequest *>(lua_newuserdata(L, sizeof(FetchRequest)));
  udFetchRequest->data = fetch;
  fetch->userData = udFetchRequest;
  luaL_setmetatable(L, FETCH_REQUEST_NAME);
  return 1;
}

int L_fetch__gc(lua_State *L) {
  auto udFetchRequest =
      static_cast<FetchRequest *>(luaL_checkudata(L, 1, FETCH_REQUEST_NAME));
  if (udFetchRequest->data != nullptr) {
    udFetchRequest->finished = false;
    udFetchRequest->succeeded = false;
    udFetchRequest->data->userData = nullptr;
    emscripten_fetch_close(udFetchRequest->data);
    udFetchRequest->data = nullptr;
  }

  return 0;
}
#endif

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

#if defined(__EMSCRIPTEN__)
  lua_pushcfunction(L, L_fetch);
  lua_setfield(L, -2, "fetch");

  lua_pushcfunction(L, L_getFetchRequest);
  lua_setfield(L, -2, "getFetchRequest");

  lua_pushcfunction(L, L_freeFetchRequest);
  lua_setfield(L, -2, "freeFetchRequest");
#endif

  return 1;
}

} // namespace

namespace hello::lua::utils {
void openlibs(lua_State *L) {
#if defined(__EMSCRIPTEN__)
  luaL_newmetatable(L, FETCH_REQUEST_NAME);
  lua_pushcfunction(L, L_fetch__gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);
#endif

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
