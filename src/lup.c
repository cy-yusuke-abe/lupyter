#include "lup.h"


/*
 * Lua prompt handling
 */
const char *stringified = "stringified_arguments";
const char *stringifier = "stringify_argument_list";


int stringifier_LuaCB(lua_State *L) {
   luaL_Buffer B;
   luaL_buffinit(L, &B);

   int top = lua_gettop(L);
   for (int i = 1; i <= top; i++) {
      if (lua_isnil(L, i)) {
         luaL_addstring(&B, "nil");
      }
      else {
         // gross
         // because lua_tostring blows
         lua_getglobal(L, "tostring");
         lua_pushvalue(L, i);
         if (lua_pcall(L, 1, 1, 0) != 0) {
            printf("fail\n");
            exit(1);
         }
         const char *str = lua_tostring(L, -1);
         size_t str_len = strlen(str);
         char t[str_len + 1];
         memcpy(t, str, str_len + 1);
         lua_pop(L, 1);
         luaL_addstring(&B, t);
      }
      if (i < top) {
         luaL_addchar(&B, '\t');
      }
   }
   luaL_pushresult(&B);
   if (lua_isnil(L, -1)) return 0;
   lua_getfield(L, LUA_REGISTRYINDEX, stringified);
   if (!lua_isnil(L, -1)) {
      const char *previous = lua_tostring(L, -1);
      const char *current = lua_tostring(L, -2);
      if (strcmp(current, "") == 0) {
          return 0;
      }
      lua_pushfstring(L, "%s\n%s", previous, current);
      lua_replace(L, -3);
      lua_pop(L, 1);
   }
   else {
      lua_pop(L, 1);
   }
   lua_setfield(L, LUA_REGISTRYINDEX, stringified);
   return 0;
}


char *process_chunk(lua_State *L, const char *chunk) {
   char *c = NULL;
   // safety
   if (1) {
      lua_getfield(L, LUA_REGISTRYINDEX, stringifier);
      lua_setglobal(L, stringifier);
   }
   lua_pushfstring(L, "%s(%s)", stringifier, chunk);
   luaL_loadstring(L, lua_tostring(L, -1));
   if (lua_pcall(L, 0, 1, 0) != 0) {
      lua_settop(L, 0);
      if (luaL_dostring(L, chunk) != 0) {
         lua_setfield(L, LUA_REGISTRYINDEX, stringified);
      }
   }

   // here we grab the string out of the registry,
   // copy it into a malloc char array, and nil out
   // the registry.
   lua_getfield(L, LUA_REGISTRYINDEX, stringified);
   if (!lua_isnil(L, -1)) {
      lua_pushnil(L);
      lua_setfield(L, LUA_REGISTRYINDEX, stringified);
      const char *str = lua_tostring(L, -1);
      size_t str_len = strlen(str);
      c = (char*)malloc(str_len + 1);
      memcpy(c, str, str_len + 1);
   }
   lua_settop(L, 0);
   return c;
}


void init(lua_State *L) {
   lua_pushcfunction(L, stringifier_LuaCB);
   lua_setfield(L, LUA_REGISTRYINDEX, stringifier);
   lua_pushcfunction(L, stringifier_LuaCB);
   lua_setglobal(L, "print");
}
