#ifndef __LUA_UTILS_HPP__
#define __LUA_UTILS_HPP__

#include "lua_common.hpp"
namespace hello::lua {
void openlibs(lua_State *L);
int docall(lua_State *L, int narg, int nres = LUA_MULTRET);
int dostring(lua_State *L, const char *const s, int narg = 0,
             int nres = LUA_MULTRET);
int report(lua_State *L, int status);
} // namespace hello::lua
#endif
