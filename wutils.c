#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define MAX_KEY MAX_PATH

#define eq(s1,s2) (strcmp(s1,s2)==0)

typedef int Ref;

Ref make_ref(lua_State *L, int idx) {
  lua_pushvalue(L,idx);
  return luaL_ref(L,LUA_REGISTRYINDEX);
}

void release_ref(lua_State *L, Ref ref) {
  luaL_unref(L,LUA_REGISTRYINDEX,ref);
}

int push_ref(lua_State *L, Ref ref) {
  lua_rawgeti(L,LUA_REGISTRYINDEX,ref);
  return 1;
}

const char *last_error(int err) {
  static char errbuff[256];
  int sz;
  if (err == 0)
    err = GetLastError();
  sz = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,err,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    errbuff, 256, NULL );
  errbuff[sz-2] = '\0'; // strip the \r\n
  return errbuff;
}

int push_error_msg(lua_State *L, const char *msg) {
  lua_pushnil(L);
  lua_pushstring(L,msg);
  return 2;
}

int push_error(lua_State *L) {
  return push_error_msg(L,last_error(0));
}

int push_ok(lua_State *L) {
  lua_pushboolean(L,1);
  return 1;
}

int push_bool(lua_State *L, int bval) {
  if (bval) {
    return push_ok(L);
  } else {
    return push_error(L);
  }
}

void throw_error(lua_State *L, const char *msg) {
  OutputDebugString(last_error(0));
  OutputDebugString(msg);
  lua_pushstring(L,msg);
  lua_error(L);
}

BOOL call_lua_direct(lua_State *L, Ref ref, int idx, const char *text, int discard) {
  BOOL res,ipush = 1;
  if (idx < 0)
    lua_pushvalue(L,idx);
  else if (idx > 0)
    lua_pushinteger(L,idx);
  else
    ipush = 0;
  push_ref(L,ref);
  if (idx != 0)
    lua_pushvalue(L,-2);
  if (text != NULL) {
    lua_pushstring(L,text);
    ++ipush;
  }
  lua_call(L, ipush, 1);
  res = lua_toboolean(L,-1);
  if (discard) {
    release_ref(L,ref);
  }
  return res;
}

// Calling back to Lua /////
// For console applications, we just use a mutex to ensure that Lua will not
// be re-entered, but if use_gui() is called, we use a message window to
// make sure that the callback happens on the main GUI thread.

typedef struct {
  lua_State *L;
  Ref ref;
  int idx;
  const char *text;
  int discard;
} LuaCallParms;

#define MY_INTERNAL_LUA_MESSAGE WM_USER+42

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  WNDPROC lpPrevWndProc;
  if (uMsg == MY_INTERNAL_LUA_MESSAGE) {
    BOOL res;
    LuaCallParms *P  = (LuaCallParms*)lParam;
    res = call_lua_direct(P->L,P->ref,P->idx,P->text,P->discard);
    free(P);
    return res;
  }

  lpPrevWndProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (lpPrevWndProc)
    return CallWindowProc(lpPrevWndProc, hwnd, uMsg, wParam, lParam);

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static BOOL s_use_mutex = TRUE;
static HWND hMessageWin = NULL;

void make_message_window() {
  if (hMessageWin == NULL) {
    LONG_PTR subclassedProc;
    s_use_mutex = FALSE;
    hMessageWin = CreateWindow(
      "STATIC", "winapi_Spawner_Dispatcher",
      0, 0, 0, 0, 0, 0, 0, GetModuleHandle(NULL), 0
    );
    subclassedProc = SetWindowLongPtr(hMessageWin, GWLP_WNDPROC, (LONG_PTR)WndProc);
    SetWindowLongPtr(hMessageWin, GWLP_USERDATA, subclassedProc);
  }
}


// this is a useful function to call a Lua function within an exclusive
// mutex lock. There are two parameters:
//
// - the first can be zero, negative or postive. If zero, nothing happens. If
// negative, it's assumed to be an index to a value on the stack; if positive,
// assumed to be an integer value.
// - the second can be NULL or some text. If NULL, nothing is pushed.
//
BOOL call_lua(lua_State *L, Ref ref, int idx, const char *text, int discard) {
  static HANDLE hLuaMutex = NULL;
  BOOL res;
  if (hLuaMutex == NULL) {
    hLuaMutex = CreateMutex(NULL,FALSE,NULL);
  }
  WaitForSingleObject(hLuaMutex,INFINITE);
  if (s_use_mutex) {
    res = call_lua_direct(L,ref,idx,text,discard);
  } else {
    LuaCallParms *parms = (LuaCallParms*)malloc(sizeof(LuaCallParms));
    parms->L = L;
    parms->ref = ref;
    parms->idx = idx;
    parms->text = text;
    parms->discard = discard;
    PostMessage(hMessageWin,MY_INTERNAL_LUA_MESSAGE,0,(LPARAM)parms);
    res = FALSE; // for now
  }
  ReleaseMutex(hLuaMutex);
  return res;
}

static int current_encoding = CP_ACP;

void set_encoding(int e) {
  current_encoding = e;
}

int get_encoding() {
  return current_encoding;
}

LPWSTR wstring_buff(LPCSTR text, LPWSTR wbuf, int bufsz) {
  int res = MultiByteToWideChar(
    current_encoding, 0,
    text,-1,
    wbuf,bufsz);
  if (res != 0) {
    return wbuf;
  } else {
    return NULL; // how to indicate error, hm??
  }
}


int push_wstring(lua_State *L,LPCWSTR us) {
  int len = wcslen(us);
  int osz = 3*len;
  char *obuff = malloc(osz);
  int res = WideCharToMultiByte(
    current_encoding, 0,
    us,len,
    obuff,osz,
    NULL,NULL);
  if (res == 0) {
    free(obuff);
    return push_error(L);
  } else {
    lua_pushlstring(L,obuff,res);
    free(obuff);
    return 1;
  }
}

static HKEY predefined_keys(LPCSTR key) {
  #define check(predef) if (eq(key,#predef)) return predef;
  check(HKEY_CLASSES_ROOT);
  check(HKEY_CURRENT_CONFIG);
  check(HKEY_CURRENT_USER);
  check(HKEY_LOCAL_MACHINE);
  check(HKEY_USERS);
  #undef check
  return NULL;
}

#define SLASH '\\'

HKEY split_registry_key(LPCSTR path, char *keypath) {
  char key[MAX_KEY];
  LPCSTR slash = strchr(path,SLASH);
  int i = (int)((DWORD_PTR)slash - (DWORD_PTR)path);
  strncpy(key,path,i);
  key[i] = '\0';
  strcpy(keypath,path+i+1);
  return predefined_keys(key);
}
