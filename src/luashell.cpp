/**
 * @file luashell.cpp
 * @brief A very simple Lua interpreter + shell in C++
 * @version 0.3.1
 * @author Alexandre Martos
 * @email contact@amartos.fr
 * @copyright MIT license
 */

#include <cstdarg>
#include <iostream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "lua5.4/lua.hpp"

#include "readline/readline.h"
#include "readline/history.h"

/**
 * @enum GlobalsIndex
 * @since 0.2.1
 * @brief Index of components in the #GLOBALS variable.
 *
 * The values are:
 * - #XDG: a XDG environment variable for a path
 * - #XDGDEF: a default value if the XDG environment variable is
 *   empty/undefined
 * - #VAL: a value for the global; if #XDG is non-@c NULL, this
 *   represents the file name;
 * - #VAR: the global's variable name in Lua
 */
enum GlobalsIndex {XDG, XDGDEF, VAL, VAR};

/**
 * @var GLOBALS
 * @since 0.2.0
 * @brief Shell predefined globals.
 * @warning This table must finish with a sentinel (the check is done
 * on the global's variable name).
 */
static const char* const GLOBALS[][4] {
    {"XDG_CONFIG_HOME", "~/.config", "config.lua", "CONFFILE"},
    {"XDG_DATA_HOME", "~/.local/share", "history", "HISTFILE"},
    {NULL, NULL, ">>> ", "PROMPT"},
    {NULL, NULL, "", "BANNER"},
    {NULL,}
};

/**
 * @since 0.3.1
 * @brief Build a path string and make the directory component.
 * @warning Needs a sentinel NULL value.
 * @param root The root component of the path.
 * @param ... Additional components to append to the path.
 * @return The built path string.
 */
 __attribute__((sentinel))
std::string mkpath(const char* root, ...)
{
    fs::path path = root ? root : "";
    va_list components {};
    va_start(components, root);
    while((root = va_arg(components, const char*)))
        path /= root;
    va_end(components);
    fs::create_directories(path.parent_path());
    std::string retval {path.c_str()};
    return retval;
}

/**
 * @since 0.2.0
 * @brief Generate the shell's globals.
 * @param L Lua's state..
 */
static void lua_setGlobals(lua_State *L)
{
    const char* envval {};
    std::string value {};
    std::string envvar {"LUASHELL_"};
    for (auto i = 0; GLOBALS[i][VAR]; ++i, envvar = "LUASHELL_") {
        envvar += GLOBALS[i][VAR];
        envval  = std::getenv(envvar.c_str());

        if (envval && *envval)
            value = GLOBALS[i][XDG] ? mkpath(envval, NULL) : envval;
        else if (GLOBALS[i][XDG]) {
            envval = std::getenv(GLOBALS[i][XDG]);
            envval = envval && *envval ? envval : GLOBALS[i][XDGDEF];
            value  = mkpath(envval, "luashell", GLOBALS[i][VAL], NULL);
        }
        else value = GLOBALS[i][VAL];

        lua_pushstring(L, value.c_str());
        lua_setglobal(L, GLOBALS[i][VAR]);
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
 * @param L Lua's stack.
 */
static void shell(lua_State *L)
{
    char* buffer {};
    read_history(lua_global(L, "HISTFILE").c_str());
    std::cout << lua_global(L, "BANNER") << "\n";
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
