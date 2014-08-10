/*
 * luaiconv - Performs character set conversions in Lua
 * (c) 2005-08 Alexandre Erwin Ittner <aittner@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * If you use this package in a product, an acknowledgment in the product
 * documentation would be greatly appreciated (but it is not required).
 *
 * $Id: luaiconv.c 54 2008-05-19 00:37:56Z dermeister $
 *
 */


#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>

#include <iconv.h>
#include <errno.h>

#define LIB_NAME                "iconv"
#define LIB_VERSION             LIB_NAME " r5"
#define ICONV_TYPENAME          "iconv_t"


/* Compatibility between Lua 5.1+ and Lua 5.0 */
#ifndef LUA_VERSION_NUM
#define LUA_VERSION_NUM 0
#endif
#if LUA_VERSION_NUM < 501
#define luaL_register(a, b, c) luaL_openlib((a), (b), (c), 0)
#endif


/* Emulates lua_(un)boxpointer from Lua 5.0 (don't exists on Lua 5.1) */
#define boxptr(L, p)   (*(void**)(lua_newuserdata(L, sizeof(void*))) = (p))
#define unboxptr(L, i) (*(void**)(lua_touserdata(L, i)))

/* Table assumed on top */
#define tblseticons(L, c, v)    \
    lua_pushliteral(L, c);      \
    lua_pushnumber(L, v);       \
    lua_settable(L, -3);



#define ERROR_NO_MEMORY     1
#define ERROR_INVALID       2
#define ERROR_INCOMPLETE    3
#define ERROR_UNKNOWN       4


static void push_iconv_t(lua_State *L, iconv_t cd) {
    boxptr(L, cd);
    luaL_getmetatable(L, ICONV_TYPENAME);
    lua_setmetatable(L, -2);
}


static iconv_t get_iconv_t(lua_State *L, int i) {
    if (luaL_checkudata(L, i, ICONV_TYPENAME) != NULL) {
        iconv_t cd = unboxptr(L, i);
        if (cd == (iconv_t) NULL)
            luaL_error(L, "attempt to use an invalid " ICONV_TYPENAME);
        return cd;
    }
    luaL_typerror(L, i, ICONV_TYPENAME);
    return NULL;
}


static int Liconv_open(lua_State *L) {
    const char *fromcode = luaL_checkstring(L, 1);
    const char *tocode = luaL_checkstring(L, 2);
    iconv_t cd = iconv_open(tocode, fromcode);
    if (cd != (iconv_t)(-1))
        push_iconv_t(L, cd);    /* ok */
    else
        lua_pushnil(L);         /* erro */
    return 1;
}


static int Liconv(lua_State *L) {
    iconv_t cd = get_iconv_t(L, 1);
    size_t ibleft = lua_strlen(L, 2);
    char *inbuf = (char*) luaL_checkstring(L, 2);
    char *outbuf;
    char *outbufs;
    size_t obsize = (ibleft > 256) ? ibleft : 256; 
    size_t obleft = obsize;
    size_t ret = -1;
    int hasone = 0;

    outbuf = (char*) malloc(obsize * sizeof(char));
    if (outbuf == NULL) {
        lua_pushstring(L, "");
        lua_pushnumber(L, ERROR_NO_MEMORY);
        return 2;
    }
    outbufs = outbuf;

    do {
        ret = iconv(cd, &inbuf, &ibleft, &outbuf, &obleft);
        if (ret == (size_t)(-1)) {
            lua_pushlstring(L, outbufs, obsize - obleft);
            if (hasone == 1)
                lua_concat(L, 2);
            hasone = 1;
            if (errno == EILSEQ) {
                lua_pushnumber(L, ERROR_INVALID);
                free(outbufs);
                return 2;   /* Invalid character sequence */
            } else if (errno == EINVAL) {
                lua_pushnumber(L, ERROR_INCOMPLETE);
                free(outbufs);
                return 2;   /* Incomplete character sequence */
            } else if (errno == E2BIG) {
                obleft = obsize;    
                outbuf = outbufs;
            } else {
                lua_pushnumber(L, ERROR_UNKNOWN);
                free(outbufs);
                return 2; /* Unknown error */
            }
        }
    } while (ret != (size_t) 0);

    lua_pushlstring(L, outbufs, obsize - obleft);
    if (hasone == 1)
        lua_concat(L, 2);
    free(outbufs);
    return 1;   /* Done */
}



#ifdef HAS_ICONVLIST /* a GNU extension? */

static int push_one(unsigned int cnt, char *names[], void *data) {
    lua_State *L = (lua_State*) data;
    int n = (int) lua_tonumber(L, -1);
    int i;

    /* Stack: <tbl> n */
    lua_remove(L, -1);    
    for (i = 0; i < cnt; i++) {
        /* Stack> <tbl> */
        lua_pushnumber(L, n++);
        lua_pushstring(L, names[i]);
        /* Stack: <tbl> n <str> */
        lua_settable(L, -3);
    }
    lua_pushnumber(L, n);
    /* Stack: <tbl> n */
    return 0;   
}


static int Liconvlist(lua_State *L) {
    lua_newtable(L);
    lua_pushnumber(L, 1);

    /* Stack:   <tbl> 1 */
    iconvlist(push_one, (void*) L);

    /* Stack:   <tbl> n */
    lua_remove(L, -1);
    return 1;
}

#endif


static int Liconv_close(lua_State *L) {
    iconv_t cd = get_iconv_t(L, 1);
    if (iconv_close(cd) == 0)
        lua_pushboolean(L, 1);  /* ok */
    else
        lua_pushnil(L);         /* erro */
    return 1;
}


static const luaL_reg inconvFuncs[] = {
    { "open",   Liconv_open },
    { "iconv",  Liconv },
#ifdef HAS_ICONVLIST
    { "list",   Liconvlist },
#endif
    { NULL, NULL }
};


static const luaL_reg iconvMT[] = {
    { "__gc", Liconv_close },
    { NULL, NULL }
};


int luaopen_iconv(lua_State *L) {
    luaL_register(L, LIB_NAME, inconvFuncs);

    tblseticons(L, "ERROR_NO_MEMORY",   ERROR_NO_MEMORY);
    tblseticons(L, "ERROR_INVALID",     ERROR_INVALID);
    tblseticons(L, "ERROR_INCOMPLETE",  ERROR_INCOMPLETE);
    tblseticons(L, "ERROR_UNKNOWN",     ERROR_UNKNOWN);

    lua_pushliteral(L, "VERSION");
    lua_pushstring(L, LIB_VERSION);
    lua_settable(L, -3);

    luaL_newmetatable(L, ICONV_TYPENAME);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_settable(L, -3);
    luaL_openlib(L, NULL, iconvMT, 0);
    lua_pop(L, 1);

    return 1;
}
