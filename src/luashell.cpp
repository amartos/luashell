/**
 * @file luashell.cpp
 * @brief A very simple Lua interpreter + shell in C++
 * @version 0.2.1
 * @author Alexandre Martos
 * @email contact@amartos.fr
 * @copyright MIT license
 */

#include <iostream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "lua5.4/lua.hpp"

#include "readline/readline.h"
#include "readline/history.h"

/**
 * @var GLOBALS
 * @since 0.2.0
 * @brief Shell predefined globals.
 *
 * The values are:
 * - an environment variable for a path
 * - a default value if the environment variable is empty/undefined
 * - a value for the global; if the first and second values are
 * non-@c NULL, this is a file name; the remaining of the path are
 * directories
 * - the global's variable name in Lua
 *
 * This table must finish with a sentinel (the check is done on the
 * global's variable name).
 */
static const char* const GLOBALS[][4] {
    {"XDG_CONFIG_HOME", "~/.config", "config.lua", "CONFFILE"},
    {"XDG_DATA_HOME", "~/.local/share", "history", "HISTFILE"},
    {NULL, NULL, ">>> ", "PROMPT"},
    {NULL, NULL, "", "BANNER"},
    {NULL,}
};

/**
 * @since 0.2.0
 * @brief Generate the shell's globals.
 * @param L Lua's state..
 */
static void lua_setGlobals(lua_State *L)
{
    const char* value {};
    fs::path path {};
    for (auto i = 0; GLOBALS[i][3]; ++i) {
        value = GLOBALS[i][0];
        if ((value && (value = std::getenv(value)) && *value) || (value = GLOBALS[i][1])) {
            path  = value;
            path /= "/luashell/";
            fs::create_directories(path);
            path /= GLOBALS[i][2];
            value = path.c_str();
        }
        else value = GLOBALS[i][2];

        lua_pushstring(L, value);
        lua_setglobal(L, GLOBALS[i][3]);
    }
}

/**
 * @since 0.2.0
 * @brief Get the global string value associated to @p key.
 * @param L Lua's state.
 * @param key The global's name.
 * @return The string value associated to @p key.
 */
std::string lua_global(lua_State *L, const char* key)
{
    lua_getglobal(L, key);
    std::string value {};
    if (lua_isfunction(L, -1)) lua_pcall(L, 0, 1, 0);
    value = lua_tostring(L, -1);
    lua_pop(L, 1);
    return value;
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
    read_history(lua_global(L, "HISTFILE").c_str());
    std::cout << lua_global(L, "BANNER");
    while((buffer = readline(lua_global(L, "PROMPT").c_str()))) {
        lua_print(L, luaL_dostring(L, buffer));
        if (buffer[0] && buffer[0] != ' ') add_history(buffer);
        free(buffer);
    }
    write_history(lua_global(L, "HISTFILE").c_str());
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
    lua_setGlobals(L);
    std::string conffile {lua_global(L, "CONFFILE")};
    if (fs::exists(conffile) && luaL_dofile(L, conffile.c_str()) && argc > 1)
        lua_print(L, 1);

    argc > 1 ? scripts(L, ++argv) : shell(L);
    lua_close(L);
    return 0;
}
