/**
 * @file luashell.cpp
 * @brief A very simple Lua interpreter + shell in C++
 * @version 0.1.0
 * @author Alexandre Martos
 * @email contact@amartos.fr
 * @copyright MIT license
 */

#include <iostream>
#include "lua5.4/lua.hpp"

extern "C" {
#include "readline/readline.h"
#include "readline/history.h"
}

/**
 * @since 0.1.0
 * @brief Print the last value on Lua's stack.
 * @note If @p status is not #LUA_OK, the printing is done at stderr.
 * @param L Lua's stack.
 * @param status The last lua's command execution status.
 */
static void lua_print(lua_State *L, int status)
{
    if (lua_gettop(L) == 0) return;
    std::ostream& stream = status == LUA_OK ? std::cout : std::cerr;
    stream << lua_tostring(L, -1) << "\n";
    lua_pop(L, 1);
}

/**
 * @since 0.1.0
 * @brief Sequentially execute all the Lua scripts given.
 * @param L Lua's stack.
 * @param scripts All the scripts' paths.
 */
static void scripts(lua_State *L, const char* scripts[])
{
    while(*scripts) lua_print(L, luaL_dofile(L, *scripts++));
}

/**
 * @since 0.1.0
 * @brief Fire up a simple Lua shell.
 * @note Send #EOF (ctrl + d) to exit.
 * @warning The shell does not save the sessions' history.
 * @param L Lua's stack.
 */
static void shell(lua_State *L)
{
    char* buffer {};
    while((buffer = readline(">>> "))) {
        lua_print(L, luaL_dostring(L, buffer));
        if (buffer[0]) add_history(buffer);
        free(buffer);
    }
}

/**
 * @since 0.1.0
 * @brief main function.
 * @param argc The number of commandline arguments.
 * @param argv The commandline arguments.
 */
int main(int argc, const char* argv[])
{
    lua_State* L { luaL_newstate() };
    luaL_openlibs(L);
    argc > 1 ? scripts(L, ++argv) : shell(L);
    lua_close(L);
    return 0;
}
