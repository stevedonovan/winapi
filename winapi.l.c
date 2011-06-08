/***
A useful set of Windows API functions.

 * Enumerating and accessing windows, including sending keys.
 * Enumerating processes and querying their program name, memory used, etc.
 * Reading and Writing to the Registry
 * Copying and moving files, and showing drive information.
 * Lauching processes and opening documents.
 * Monitoring filesystem changes.

@author Steve Donovan  (steve.j.donovan@gmail.com)
@copyright 2011
@license MIT/X11
@module winapi
*/
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#ifdef __GNUC__
#include <winable.h> /* GNU GCC specific */
#endif
#include <psapi.h>

#define eq(s1,s2) (strcmp(s1,s2)==0)

#define WBUFF 2048
#define MAX_SHOW 100
#define MAX_KEY MAX_PATH
#define THREAD_STACK_SIZE (1024 * 1024)
#define MAX_PROCESSES 1024
#define MAX_KEYS 512
#define FILE_BUFF_SIZE 2048
#define MAX_WATCH 20

static char buff[WBUFF];

module "winapi" {

typedef int Ref;

static Ref make_ref(lua_State *L, int idx) {
  lua_pushvalue(L,idx);
  return luaL_ref(L,LUA_REGISTRYINDEX);
}

static void release_ref(lua_State *L, Ref ref) {
  luaL_unref(L,LUA_REGISTRYINDEX,ref);
}

static int push_ref(lua_State *L, Ref ref) {
  lua_rawgeti(L,LUA_REGISTRYINDEX,ref);
  return 1;
}

static const char *last_error(int err) {
  static char buff[256];
  int sz;
  if (err == 0)
    err = GetLastError();
  sz = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,err,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    buff, 256, NULL );
  buff[sz-2] = '\0'; // strip the \r\n
  return buff;
}

static int push_error(lua_State *L) {
  lua_pushnil(L);
  lua_pushstring(L,last_error(0));
  return 2;
}

static void throw_error(lua_State *L, const char *msg) {
  OutputDebugString(last_error(0));
  OutputDebugString(msg);
  lua_pushstring(L,msg);
  lua_error(L);
}

static BOOL call_lua_direct(lua_State *L, Ref ref, int idx, const char *text, int discard) {
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

HWND make_message_window() {
  LONG_PTR subclassedProc;
  HWND hwndDispatcher = CreateWindow(
    "STATIC", "winapi_Spawner_Dispatcher",
    0, 0, 0, 0, 0, 0, 0, GetModuleHandle(NULL), 0
  );
  subclassedProc = SetWindowLongPtr(hwndDispatcher, GWLP_WNDPROC, (LONG_PTR)WndProc);
  SetWindowLongPtr(hwndDispatcher, GWLP_USERDATA, subclassedProc);
  return hwndDispatcher;
}

static BOOL s_use_mutex = TRUE;
static HWND hMessageWin = NULL;

// this is a useful function to call a Lua function within an exclusive
// mutex lock. There are two parameters:
//
// - the first can be zero, negative or postive. If zero, nothing happens. If
// negative, it's assumed to be an index to a value on the stack; if positive,
// assumed to be an integer value.
// - the second can be NULL or some text. If NULL, nothing is pushed.
//
static BOOL call_lua(lua_State *L, Ref ref, int idx, const char *text, int discard) {
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

/// a class representing a Window.
// @type Window
class Window {
  HWND hwnd;

  constructor (HWND h) {
    this->hwnd = h;
  }

  static lua_State *sL;

  static BOOL CALLBACK enum_callback(HWND hwnd,LPARAM data) {
    push_ref(sL,(Ref)data);
    push_new_Window(sL,hwnd);
    lua_call(sL,1,0);
    return TRUE;
  }

  /// the handle of this window.
  // @function handle
  def handle() {
    lua_pushinteger(L,(DWORD_PTR)this->hwnd);
    return 1;
  }

  /// get the window text.
  // @function get_text
  def get_text() {
    GetWindowText(this->hwnd,buff,sizeof(buff));
    lua_pushstring(L,buff);
    return 1;
  }

  /// set the window text.
  // @function set_text
  def set_text(Str text) {
    SetWindowText(this->hwnd,text);
    return 0;
  }

  /// Change the visibility, state etc
  // @param flags one of SW_SHOW, SW_MAXIMIZE, etc
  // @function show
  def show(Int flags = SW_SHOW) {
    ShowWindow(this->hwnd,flags);
    return 0;
  }

  /// get the position in pixels
  // @return left position
  // @return top position
  // @function get_position
  def get_position() {
    RECT rect;
    GetWindowRect(this->hwnd,&rect);
    lua_pushinteger(L,rect.left);
    lua_pushinteger(L,rect.top);
    return 2;
  }

  /// get the bounds in pixels
  // @return width
  // @return height
  // @function get_bounds
  def get_bounds() {
    RECT rect;
    GetWindowRect(this->hwnd,&rect);
    lua_pushinteger(L,rect.right - rect.left);
    lua_pushinteger(L,rect.bottom - rect.top);
    return 2;
  }

  /// is this window visible?
  // @function is_visible
  def is_visible() {
    lua_pushboolean(L,IsWindowVisible(this->hwnd));
    return 1;
  }

  /// destroy this window.
  // @function destroy
  def destroy () {
    DestroyWindow(this->hwnd);
    return 0;
  }

  /// resize this window.
  // @param x0 left
  // @param y0 top
  // @param w width
  // @param h height
  // @function resize
  def resize(Int x0, Int y0, Int w, Int h) {
    MoveWindow(this->hwnd,x0,y0,w,h,TRUE);
    return 0;
  }

  /// send a message.
  // @param msg the message
  // @param wparam
  // @param lparam
  // @return the result
  // @function send_message
  def send_message(Int msg, Int wparam, Int lparam) {
    lua_pushinteger(L,SendMessage(this->hwnd,msg,wparam,lparam));
    return 1;
  }

  /// enumerate all child windows.
  // @param a callback which to receive each window object
  // @function enum_children
  def enum_children(Value callback) {
    Ref ref;
    sL = L;
    ref = make_ref(L,callback);
    EnumChildWindows(this->hwnd,&enum_callback,ref);
    release_ref(L,ref);
    return 0;
  }

  /// get the parent window.
  // @function get_parent
  def get_parent() {
    push_new_Window(L,GetParent(this->hwnd));
    return 1;
  }

  /// get the name of the program owning this window.
  // @function get_module_filename
  def get_module_filename() {
    int sz = GetWindowModuleFileName(this->hwnd,buff,sizeof(buff));
    buff[sz] = '\0';
    lua_pushstring(L,buff);
    return 1;
  }

  /// get the window class name.
  // Useful to find all instances of a running program, when you
  // know the class of the top level window.
  // @function get_class_name
  def get_class_name() {
    GetClassName(this->hwnd,buff,sizeof(buff));
    lua_pushstring(L,buff);
    return 1;
  }

  /// bring this window to the foreground.
  // @function set_foreground
  def set_foreground () {
    lua_pushboolean(L,SetForegroundWindow(this->hwnd));
    return 1;
  }


  /// this window as string (up to 100 chars).
  // @function __tostring
  def __tostring() {
    GetWindowText(this->hwnd,buff,sizeof(buff));
    if (strlen(buff) > MAX_SHOW) {
      strcpy(buff+MAX_SHOW,"...");
    }
    lua_pushstring(L,buff);
    return 1;
  }

  def __eq(Window other) {
    lua_pushboolean(L,this->hwnd == other->hwnd);
    return 1;
  }

}

/// Manipulating Windows.
// @section Windows

/// find a window based on classname and caption
// @param cname class name (may be nil)
// @param wname caption (may be nil)
// @return a window object
// @function find_window
def find_window(StrNil cname, StrNil wname) {
  HWND hwnd;
  hwnd = FindWindow(cname,wname);
  push_new_Window(L,hwnd);
  return 1;
}

/// makes a function that matches against window text
// @param text
// @function match_name

/// makes a function that matches against window class name
// @param text
// @function match_class

/// find a window using a condition function.
// @param match will return true when its argument is the desired window
// @return window object
// @function find_window_ex

/// return all windows matching a condition.
// @param match will return true when its argument is the desired window
// @return a list of window objects
// @function find_all_windows

/// find a window matching the given text.
// @param text the pattern to match against the caption
// @return a window object.
// @function find_window_match

/// currently foreground window.
// @return a window object
// @function foreground_window
def foreground_window() {
  push_new_Window(L, GetForegroundWindow());
  return 1;
}

/// the desktop window.
// @return a window object
// @usage winapi.desktop_window():get_bounds()
// @function desktop_window
def desktop_window() {
  push_new_Window(L, GetDesktopWindow());
  return 1;
}

/// enumerate over all top-level windows.
// @param callback a function to receive each window object
// @function enum_windows
def enum_windows(Value callback) {
  Ref ref;
  sL = L;
  ref  = make_ref(L,callback);
  EnumWindows(&enum_callback,ref);
  release_ref(L,ref);
  return 0;
}

/// route callback dispatch through a message window.
// @function use_gui
def use_gui() {
  s_use_mutex = FALSE;
  if (hMessageWin == NULL) { // important to be created in the main thread
    hMessageWin = make_message_window();
  }
  return 0;
}

static INPUT *add_input(INPUT *pi, WORD vkey, BOOL up) {
  pi->type = INPUT_KEYBOARD;
  pi->ki.dwFlags =  up ? KEYEVENTF_KEYUP : 0;
  pi->ki.wVk = vkey;
  return pi+1;
}

// The Windows SendInput() is a low-level function, and you have to
// simulate things like uppercase directly. Repeated characters need
// an explicit 'key up' keystroke to work.
// see http://stackoverflow.com/questions/2167156/sendinput-isnt-sending-the-correct-shifted-characters
// this is a case where we have to convert the parameter directly, since
// it may be an integer (virtual key code) or string of characters.

/// send a string or virtual key to the active window.
// @param text either a key (like winapi.VK_SHIFT) or a string
// @return number of keys sent, or nil if an error
// @return any error string
// @function send_input
def send_input () {
  const char *text;
  int vkey, len = MAX_KEYS;
  UINT res;
  SHORT last_vk = 0;
  INPUT *input, *pi;
  if (lua_isnumber(L,1)) {
    INPUT inp;
    ZeroMemory(&inp,sizeof(INPUT));
    vkey = lua_tointeger(L,1);
    add_input(&inp,vkey,lua_toboolean(L,2));
    SendInput(1,&inp,sizeof(INPUT));
    return 0;
  } else {
    text = lua_tostring(L,1);
    if (text == NULL) {
      lua_pushnil(L);
      lua_pushliteral(L,"not a string or number");
      return 2;
    }
  }
  input = (INPUT *)malloc(sizeof(INPUT)*len);
  pi = input;
  ZeroMemory(input, sizeof(INPUT)*len);
  for(; *text; ++text) {
    SHORT vk = VkKeyScan(*text);
    if (last_vk == vk) {
      pi = add_input(pi,last_vk & 0xFF,TRUE);
    }
    if (vk & 0x100) pi = add_input(pi,VK_SHIFT,FALSE);
    pi = add_input(pi,vk & 0xFF,FALSE);
    if (vk & 0x100) pi = add_input(pi,VK_SHIFT,TRUE);
    last_vk = vk;
  }
  res = SendInput(((DWORD_PTR)pi-(DWORD_PTR)input)/sizeof(INPUT), input, sizeof(INPUT));
  free(input);
  if (res > 0) {
    lua_pushinteger(L,res);
    return 1;
  } else {
    return push_error(L);
  }
  return 0;
}

/// tile a group of windows.
// @param parent (can use the desktop)
// @param horiz tile vertically by default
// @param kids a table of window objects
// @param bounds a bounds table (left,top,right,bottom) - can be nil
// @function tile_windows
def tile_windows(Window parent, Boolean horiz, Value kids, Value bounds) {
  RECT rt;
  HWND *kids_arr;
  int i,n_kids;
  LPRECT lpRect = NULL;
  if (! lua_isnoneornil(L,bounds)) {
    lua_pushvalue(L,bounds);
    Int_get(rt.left,"left");
    Int_get(rt.top,"top");
    Int_get(rt.right,"right");
    Int_get(rt.bottom,"bottom");
    lua_pop(L,1);
    lpRect = &rt;
  }
  n_kids = lua_objlen(L,kids);
  kids_arr = (HWND *)malloc(sizeof(HWND)*n_kids);
  for (i = 0; i < n_kids; ++i) {
    Window *w;
    lua_rawgeti(L,kids,i+1);
    w = Window_arg(L,-1);
    kids_arr[i] = w->hwnd;
  }
  TileWindows(parent->hwnd,horiz ? MDITILE_HORIZONTAL : MDITILE_VERTICAL, lpRect, n_kids, kids_arr);
  free(kids_arr);
  return 0;
}

/// Miscelaneous functions.
// @section Miscelaneous

/// sleep and use no processing time.
// @param millisec sleep period
// @function sleep
def sleep(Int millisec) {
  Sleep(millisec);
  return 0;
}

/// show a message box.
// @param caption for dialog
// @param msg the message
// @function show_message
def show_message(Str caption, Str msg) {
  MessageBox( NULL, msg, caption, MB_OK | MB_ICONINFORMATION );
  return 0;
}

/// copy a file.
// @param src source file
// @param dest destination file
// @param fail_if_exists if true, then cannot copy onto existing file
// @function copy_file
def copy_file(Str src, Str dest, Int fail_if_exists = 0) {
  if (CopyFile(src,dest,fail_if_exists)) {
    lua_pushboolean(L,1);
    return 1;
  } else {
    return push_error(L);
  }
}

/// move a file.
// @param src source file
// @param dest destination file
// @function move_file
def move_file(Str src, Str dest) {
  if (MoveFile(src,dest)) {
    lua_pushboolean(L,1);
    return 1;
  } else {
    return push_error(L);
  }
}

/// execute a shell command.
// @param verb the action (e.g. 'open' or 'edit') can be nil.
// @param file the command
// @param parms any parameters (optional)
// @param dir the working directory (optional)
// @param show the window show flags (default is SW_SHOWNORMAL)
// @function shell_exec
def shell_exec(StrNil verb, Str file, StrNil parms, StrNil dir, Int show=SW_SHOWNORMAL) {
  DWORD_PTR res = (DWORD_PTR)ShellExecute(NULL,verb,file,parms,dir,show);
  if (res > 32) {
    lua_pushboolean(L,1);
    return 1;
  } else {
    return push_error(L);
  }
}

/// Copy text onto the clipboard.
// @param text the text
// @function put_clipboard_text
def put_clipboard_text(Str text) {
  HGLOBAL glob;
  char *p;
  if (! OpenClipboard(NULL)) {
    return push_error(L);
  }
  EmptyClipboard(); // hhmmmm
  glob = GlobalAlloc(GMEM_MOVEABLE, lua_objlen(L,1)+1);
  p = GlobalLock(glob);
  strcpy(p, text);
  GlobalUnlock(glob);
  if (SetClipboardData(CF_TEXT,glob) == NULL) {
    CloseClipboard();
    return push_error(L);
  }
  CloseClipboard();
  return 0;
}

/// Get the text on the clipboard.
// @return the text
// @function get_clipboard_text
def get_clipboard_text() {
  HGLOBAL glob;
  char *p;
  if (! OpenClipboard(NULL)) {
    return push_error(L);
  }
  glob = GetClipboardData(CF_TEXT);
  if (glob == NULL) {
    CloseClipboard();
    return push_error(L);
  }
  p = GlobalLock(glob);
  lua_pushstring(L,p);
  GlobalUnlock(glob);
  CloseClipboard();
  return 1;
}

/// A class representing a Windows process.
// this example was very helpful:
// http://msdn.microsoft.com/en-us/library/ms682623%28VS.85%29.aspx
// @type Process
class Process {
  HANDLE hProcess;
  int pid;

  constructor(Int pid, HANDLE ph) {
    if (ph) {
        this->pid = pid;
        this->hProcess = ph;
    } else {
        this->pid = pid;
        this->hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                                    PROCESS_VM_READ,
                                    FALSE, pid );
        }
  }

  /// get the name of the process.
  // @function get_process_name
  def get_process_name() {
    HMODULE hMod;
    DWORD cbNeeded;
    char modname[MAX_PATH];

    if (EnumProcessModules(this->hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
      GetModuleBaseName(this->hProcess, hMod, modname, sizeof(modname));
      lua_pushstring(L,modname);
      return 1;
    } else {
      return push_error(L);
    }
  }

  /// kill the process.
  // @function kill
  def kill() {
    TerminateProcess(this->hProcess,0);
    return 0;
  }

  /// get the working size of the process.
  // @return minimum working set size
  // @return maximum working set size.
  // @function working_size
  def working_size() {
    SIZE_T minsize, maxsize;
    GetProcessWorkingSetSize(this->hProcess,&minsize,&maxsize);
    lua_pushnumber(L,minsize/1024);
    lua_pushnumber(L,maxsize/1024);
    return 2;
  }

  /// get the start time of this process.
  // @return a table containing year, month, etc fields.
  // @function start_time
  def start_time() {
    FILETIME create,exit,kernel,user;
    SYSTEMTIME time;
    GetProcessTimes(this->hProcess,&create,&exit,&kernel,&user);
    FileTimeToSystemTime(&create,&time);
    #define set(name,val) lua_pushinteger(L,val); lua_setfield(L,-2,#name);
    lua_newtable(L);
    set(year,time.wYear);
    set(month,time.wMonth);
    set(day,time.wDay);
    set(hour,time.wHour);
    set(minute,time.wMinute);
    set(second,time.wSecond);
    #undef set
    return 1;
  }

  // MS likes to be different: the 64-bit value encoded in FILETIME
  // is defined as the number of 100-nsec intervals since Jan 1, 1601 UTC
  static double fileTimeToMillisec(FILETIME *ft) {
    ULARGE_INTEGER ui;
    ui.LowPart = ft->dwLowDateTime;
    ui.HighPart = ft->dwHighDateTime;
    return (double) (ui.QuadPart/10000);
  }

  /// elapsed run time of this process.
  // @return user time in msec
  // @return system time in msec
  // @function run_times
  def run_times() {
    FILETIME create,exit,kernel,user;
    GetProcessTimes(this->hProcess,&create,&exit,&kernel,&user);
    lua_pushnumber(L,fileTimeToMillisec(&user));
    lua_pushnumber(L,fileTimeToMillisec(&kernel));
    return 2;
  }

  /// wait for this process to finish.
  // @param timeout optional timeout in millisec; defaults to waiting indefinitely.
  // @return this process object
  // @return either "OK" or "TIMEOUT"
  // @function wait
  def wait(Int timeout = 0) {
    DWORD res = WaitForSingleObject(this->hProcess, timeout == 0 ? INFINITE : timeout);
    if (res == WAIT_OBJECT_0) {
        lua_pushvalue(L,1);
        lua_pushliteral(L,"OK");
        return 2;
    } else if (res == WAIT_TIMEOUT) {
        lua_pushvalue(L,1);
        lua_pushliteral(L,"TIMEOUT");
        return 2;
    } else {
        return push_error(L);
    }
  }

  /// exit code of this process.
  // (Only makes sense if the process has in fact finished.)
  // @return exit code
  // @function exit_code
  def exit_code() {
    DWORD code;
    GetExitCodeProcess(this->hProcess, &code);
    lua_pushinteger(L,code);
    return 1;
  }

  /// close this process handle.
  // @function close
  def close() {
    CloseHandle(this->hProcess);
    this->hProcess = NULL;
    return 0;
  }

  def __gc () {
    if (this->hProcess != NULL)
      CloseHandle(this->hProcess);
    return 0;
  }
}

/// Working with processes.
// @section Processes

/// Create a process object from the id.
// @param pid the process id
// @function process
def process(Int pid) {
  push_new_Process(L,pid,NULL);
  return 1;
}

/// Process id of current process.
// @function current_pid
def current_pid() {
  lua_pushinteger(L,GetCurrentProcessId());
  return 1;
}

/// Process object of the current process.
// @function current_process
def current_process() {
  push_new_Process(L,0,GetCurrentProcess());
  return 1;
}

/// get all process ids in the system.
// @return an array of process ids.
// @function get_processes
def get_processes() {
  DWORD processes[MAX_PROCESSES], cbNeeded, nProcess;
  int i, k = 1;

  if (! EnumProcesses (processes,sizeof(processes),&cbNeeded)) {
    return push_error(L);
  }

  nProcess = cbNeeded/sizeof (DWORD);
  lua_newtable(L);
  for (i = 0; i < nProcess; i++) {
    if (processes[i] != 0) {
      lua_pushinteger(L,processes[i]);
      lua_rawseti(L,-2,k++);
    }
  }
  return 1;
}

/// Wait for a group of processes
// @param processes an array of process objects
// @param all wait for all processes to finish (default false)
// @param timeout wait upto this time in msec (default infinite)
// @function wait_for_processes
def wait_for_processes(Value processes, Boolean all, Int timeout = 0) {
  int status, i;
  Process *p;
  int n = lua_objlen(L,processes);
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  if (n > MAXIMUM_WAIT_OBJECTS) {
    lua_pushnil(L);
    lua_pushliteral(L,"cannot wait on so many processes");
    return 2;
  }
  for (i = 0; i < n; i++) {
    lua_rawgeti(L,processes,i+1);
    p = Process_arg(L,-1);
    handles[i] = p->hProcess;
  }
  status = WaitForMultipleObjects(n, handles, all, timeout == 0 ? INFINITE : timeout);
  status -= WAIT_OBJECT_0 + 1;
  if (status < 1 || status > n) {
    return push_error(L);
  } else {
    lua_pushinteger(L,status);
    return 1;
  }
}

/// Class representing Windows registry keys.
// @type regkey
class regkey {
  HKEY key;

  constructor (HKEY k) {
    this->key = k;
  }

  /// set the string value of a name.
  // @param name the name
  // @param val the value
  // @function set_value
  def set_value(Str name, Str val) {
    if (RegSetValueEx(this->key,name,0,REG_SZ,val,lua_objlen(L,2)) != ERROR_SUCCESS) {
      return push_error(L);
    } else {
      lua_pushboolean(L,1);
      return 1;
    }
  }

  /// get the value and type of a name.
  // @param name the name (can be empty for the default value)
  // @return the value (either a string or a number)
  // @return the type
  // @function get_value
  def get_value(Str name = "") {
    DWORD type,size = sizeof(buff);
    if (RegQueryValueEx(this->key,name,0,&type,buff,&size) != ERROR_SUCCESS) {
      return push_error(L);
    }
    if (type == REG_BINARY || type == REG_EXPAND_SZ || type == REG_SZ) {
      lua_pushlstring(L,buff,size);
    } else {
      lua_pushnumber(L,*(unsigned long *)buff);
    }
    lua_pushinteger(L,type);
    return 2;

  }

  /// enumerate the subkeys of a key.
  // @return a table of key names
  // @function get_keys
  def get_keys() {
    int i = 0;
    LONG res;
    DWORD size;
    lua_newtable(L);
    while (1) {
      size = sizeof(buff);
      res = RegEnumKeyEx(this->key,i,buff,&size,NULL,NULL,NULL,NULL);
      if (res != ERROR_SUCCESS) break;
      lua_pushstring(L,buff);
      lua_rawseti(L,-2,i+1);
      ++i;
    }
    if (res != ERROR_NO_MORE_ITEMS) {
      lua_pop(L,1);
      return push_error(L);
    }
    return 1;
  }

  /// close this key.
  // @function close
  def close() {
    RegCloseKey(this->key);
    this->key = NULL;
    return 0;
  }

  def __gc() {
    if (this->key != NULL)
      RegCloseKey(this->key);
    return 0;
  }

}

/// Registry Functions.
// @section Registry

static HKEY predefined_keys(Str key) {
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

static HKEY split_registry_key(Str path, char *keypath) {
  char key[MAX_KEY];
  const char *slash = strchr(path,SLASH);
  int i = (int)((DWORD_PTR)slash - (DWORD_PTR)path);
  strncpy(key,path,i);
  key[i] = '\0';
  strcpy(keypath,path+i+1);
  return predefined_keys(key);
}

/// Open a registry key.
// @param path the full registry key
// e.g [[HKEY\_LOCAL\_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion]]
// @param writeable true if you want to set values
// @return a regkey object
// @function open_key
def open_key(Str path, Boolean writeable) {
  HKEY hKey;
  DWORD access;
  hKey = split_registry_key(path,buff);
  if (hKey == NULL) {
    lua_pushnil(L);
    lua_pushliteral(L,"unrecognized registry key");
    return 2;
  }
  access = writeable ? KEY_ALL_ACCESS : (KEY_READ | KEY_ENUMERATE_SUB_KEYS);
  if (RegOpenKeyEx(hKey,buff,0,access,&hKey) == ERROR_SUCCESS) {
    push_new_regkey(L,hKey);
    return 1;
  } else {
    return push_error(L);
  }
}

/// Create a registry key.
// @param path the full registry key
// @return a regkey object
// @function create_key
def create_key (Str path) {
  HKEY hKey = split_registry_key(path,buff);
  if (hKey == NULL) {
    lua_pushnil(L);
    lua_pushliteral(L,"unrecognized registry key");
    return 2;
  }
  if (RegCreateKeyEx(hKey,buff,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,NULL)) {
    push_new_regkey(L,hKey);
    return 1;
  } else {
    return push_error(L);
  }
}

// These functions are all run in background threads, and a little bit of poor man's
// OOP helps here.

typedef struct {
  lua_State *L;
  Ref callback;
} LuaCallback;

void set_callback(void *lcb, lua_State *L, int idx) {
  LuaCallback *data = (LuaCallback*) lcb;
  data->L = L;
  data->callback = make_ref(L,idx);
}

BOOL call_lua_callback(void *data, int idx, Str text, int persist) {
  LuaCallback *lcb = (LuaCallback*)data;
  return call_lua(lcb->L,lcb->callback,idx,text,persist);
}

/// this represents a raw Windows file handle.
// @type File
class File {
  lua_State *L;
  Ref callback;
  HANDLE hRead;
  HANDLE hWrite;
  char *buf;
  int bufsz;

  constructor (HANDLE hread, HANDLE hwrite) {
    this->hRead = hread;
    this->hWrite = hwrite;
    this->L = L;
    this->bufsz = FILE_BUFF_SIZE;
    this->buf = malloc(this->bufsz);
  }

  /// write to a file.
  // @param s text
  // @return number of bytes written.
  // @function write
  def write(Str s) {
    DWORD bytesWrote;
    WriteFile(this->hWrite, s, lua_objlen(L,2), &bytesWrote, NULL);
    lua_pushinteger(L,bytesWrote);
    return 1;
  }

  static BOOL raw_read (File *this) {
    DWORD bytesRead = 0;
    BOOL res = ReadFile(this->hRead, this->buf, this->bufsz, &bytesRead, NULL);
    this->buf[bytesRead] = '\0';
    return res && bytesRead;
  }

  /// read from a file.
  // Please note that this is not buffered, and you will have to
  // split into lines, etc yourself.
  // @return text if successful, nil plus error otherwise.
  // @function read
  def read() {
    if (raw_read(this)) {
      lua_pushstring(L,this->buf);
      return 1;
    } else {
      return push_error(L);
    }
  }

  static void file_reader (File *this) { // background reader thread
    int n;
    do {
      n = raw_read(this);
      call_lua_callback (this,0,this->buf,! n);
    } while (n);

  }

  /// asynchronous read.
  // @param callback function that will receive each chunk of text
  // as it comes in.
  // @function read_async
  def read_async (Value callback) {
    this->callback = make_ref(L,callback);
    _beginthread(&file_reader, THREAD_STACK_SIZE,this);
    return 0;
  }

  def close() {
    CloseHandle(this->hRead);
    if (this->hWrite != this->hRead)
      CloseHandle(this->hWrite);
    free(this->buf);
    return 0;
  }

  def __gc () {
    free(this->buf);
    return 0;
  }
}

/// Launching processes.
// @section Launch

/// Spawn a process.
// @param program the command-line (program + parameters)
// @return a process object
// @return a File object
// @see File, Process
// @function spawn
def spawn(Str program) {
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), 0, 0};
  SECURITY_DESCRIPTOR sd;
  STARTUPINFO si = {
           sizeof(STARTUPINFO), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
       };
  HANDLE hPipeRead,hWriteSubProcess;
  HANDLE hRead2,hPipeWrite;
  BOOL running;
  PROCESS_INFORMATION pi;
  HANDLE hProcess = GetCurrentProcess();
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;
  InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = &sd;

  // Create pipe for output redirection
  CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0);

  // Create pipe for input redirection. In this code, you do not
  // redirect the output of the child process, but you need a handle
  // to set the hStdInput field in the STARTUP_INFO struct. For safety,
  // you should not set the handles to an invalid handle.

  hRead2 = NULL;
  CreatePipe(&hRead2, &hWriteSubProcess, &sa, 0);

  SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hWriteSubProcess, HANDLE_FLAG_INHERIT, 0);

  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;
  si.hStdInput = hRead2;
  si.hStdOutput = hPipeWrite;
  si.hStdError = hPipeWrite;

  running = CreateProcess(
        NULL,
        (char*)program,
        NULL, NULL,
        TRUE, CREATE_NEW_PROCESS_GROUP,
        NULL,
        NULL,
        &si, &pi);

  if (running) {
    CloseHandle(pi.hThread);
    CloseHandle(hRead2);
    CloseHandle(hPipeWrite);
    push_new_Process(L,pi.dwProcessId,pi.hProcess);
    push_new_File(L,hPipeRead,hWriteSubProcess);
    return 2;
  } else {
    return push_error(L);
  }
}

/// Execute a system command.
// This is like os.execute(), except that it works without ugly
// console flashing in Windows GUI applications. It additionally
// returns all text read from stdout and stderr.
// @param cmd a shell command (may include redirection, etc)
// @return status code
// @return program output
// @function execute

// Timer support //////////
typedef struct {
  lua_State *L;
  Ref callback;
  int msec;
} TimerData;

static void timer_thread(TimerData *data) { // background timer thread
  while (1) {
    Sleep(data->msec);
    if (call_lua_callback(data,0,0,0))
      break;
  }
}

/// Asynchronous Timers.
// @section Timers

/// Create an asynchronous timer.
// The callback can return true if it wishes to cancel the timer.
// @param msec interval in millisec
// @param callback a function to be called at each interval.
// @function timer
def timer(Int msec, Value callback) {
  TimerData *data = (TimerData *)malloc(sizeof(TimerData));
  data->msec = msec;
  set_callback(data,L,callback);
  _beginthread(&timer_thread,THREAD_STACK_SIZE,data);
  return 0;
}

#define PSIZE 512

typedef struct {
  lua_State *L;
  Ref callback;
  const char *pipename;
} PipeServerParms;

static void pipe_server_thread(PipeServerParms *parms) {
  while (1) {
    BOOL connected;
    HANDLE hPipe = CreateNamedPipe(
          parms->pipename,             // pipe named
          PIPE_ACCESS_DUPLEX,       // read/write access
          PIPE_WAIT,                // blocking mode
          255,
          PSIZE,                  // output buffer size
          PSIZE,                  // input buffer size
          0,                        // client time-out
          NULL);                    // default security attribute

    if (hPipe == INVALID_HANDLE_VALUE) {
      push_error(parms->L); // how to signal main thread about this?
    }
    // Wait for the client to connect; if it succeeds,
    // the function returns a nonzero value. If the function
    // returns zero, GetLastError returns ERROR_PIPE_CONNECTED.

    connected = ConnectNamedPipe(hPipe, NULL) ?
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected) {
      push_new_File(parms->L,hPipe,hPipe);
      call_lua_callback(parms,-1,0,0);
    } else {
      CloseHandle(hPipe);
    }
  }
}

/// Dealing with named pipes.
// @section Pipes

/// Open a pipe for reading and writing.
// @param pipename the pipename (default is "\\\\.\\pipe\\luawinapi")
// @function open_pipe
def open_pipe(Str pipename = "\\\\.\\pipe\\luawinapi") {
  HANDLE hPipe = CreateFile(
      pipename,
      GENERIC_READ |  // read and write access
      GENERIC_WRITE,
      0,              // no sharing
      NULL,           // default security attributes
      OPEN_EXISTING,  // opens existing pipe
      0,              // default attributes
      NULL);          // no template file
  if (hPipe == INVALID_HANDLE_VALUE) {
    return push_error(L);
  } else {
    push_new_File(L,hPipe,hPipe);
    return 1;
  }
}

/// Create a named pipe server.
// This goes into a background loop, and accepts client connections.
// For each new connection, the callback will be called with a File
// object for reading and writing to the client.
// @param callback a function that will be passed a File object
// @param pipename Must be of the form \\.\pipe\name, defaults to
// \\.\pipe\luawinapi.
// @function server
def server(Value callback, Str pipename = "\\\\.\\pipe\\luawinapi") {
  PipeServerParms *psp = (PipeServerParms*)malloc(sizeof(PipeServerParms));
  set_callback(psp,L,callback);
  psp->pipename = pipename;
  _beginthread(&pipe_server_thread,THREAD_STACK_SIZE,psp);
  return 0;
}

// Directory change notification ///////

typedef struct {
  lua_State *L;
  Ref callback;
  const char *dir;
  DWORD how;
  DWORD subdirs;
  HANDLE hDir;
  char *buff;
  int buffsize;
} FileChangeParms;

static void file_change_thread(FileChangeParms *fc) { // background file monitor thread
  while (1) {
    int next;
    DWORD bytes;
    // This fills in some gaps:
    // http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html
    if (! ReadDirectoryChangesW(fc->hDir,fc->buff,fc->buffsize,
        fc->subdirs, fc->how, &bytes,NULL,NULL))
    {
      throw_error(fc->L,"read dir changes failed");
    }
    next = 0;
    do {
      int i,sz, outchars;
      short *pfile;
      char outbuff[MAX_PATH];
      PFILE_NOTIFY_INFORMATION pni = (PFILE_NOTIFY_INFORMATION)(fc->buff+next);
      outchars = WideCharToMultiByte(
        CP_UTF8, 0,
        pni->FileName,
        pni->FileNameLength/2, // it's bytes, not number of characters!
        outbuff,sizeof(outbuff),
        NULL,NULL);
      if (outchars == 0) {
        throw_error(fc->L,"wide char conversion borked");
      }
      outbuff[outchars] = '\0';  // not null-terminated!
      call_lua_callback(fc,pni->Action,outbuff,0);
      next = pni->NextEntryOffset;
    } while (next != 0);
  }
}

/// Drive information and watching directories.
// @section Directories


/// get all the drives on this computer.
// @return a table of drive names
// @function get_logical_drives
def get_logical_drives() {
  int i, lasti = 0, k = 1;
  const char *p = buff;
  DWORD size = GetLogicalDriveStrings(sizeof(buff),buff);
  lua_newtable(L);
  for (i = 0; i < size; i++) {
    if (buff[i] == '\0') {
      lua_pushlstring(L,p, i - lasti);
      lua_rawseti(L,-2,k++);
      p = buff + i+1;
      lasti = i+1;
    }
  }
  return 1;
}

/// get the type of the given drive.
// @param root root of drive (e.g. 'c:\\')
// @return one of the following: unknown, none, removable, fixed, remote,
// cdrom, ramdisk.
// @function get_drive_type
def get_drive_type(Str root) {
  UINT res = GetDriveType(root);
  const char *type;
  switch(res) {
    case DRIVE_UNKNOWN: type = "unknown"; break;
    case DRIVE_NO_ROOT_DIR: type = "none"; break;
    case DRIVE_REMOVABLE: type = "removable"; break;
    case DRIVE_FIXED: type = "fixed"; break;
    case DRIVE_REMOTE: type = "remote"; break;
    case DRIVE_CDROM: type = "cdrom"; break;
    case DRIVE_RAMDISK: type = "ramdisk"; break;
  }
  lua_pushstring(L,type);
  return 1;
}

/// get the free disk space.
// @param root the root of the drive (e.g. 'd:\\')
// @return free space in kB
// @return total space in kB
// @function get_disk_free_space
def get_disk_free_space(Str root) {
  ULARGE_INTEGER freebytes, totalbytes;
  if (! GetDiskFreeSpaceEx(root,&freebytes,&totalbytes,NULL)) {
    return push_error(L);
  }
  lua_pushnumber(L,freebytes.QuadPart/1024);
  lua_pushnumber(L,totalbytes.QuadPart/1024);
  return 2;
}




//// start watching a directory.
// @param dir the directory
// @param how what events to monitor. Can be a sum of these flags:
//
//  * FILE_NOTIFY_CHANGE_FILE_NAME
//  * FILE_NOTIFY_CHANGE_DIR_NAME
//  * FILE_NOTIFY_CHANGE_LAST_WRITE
//
// @param subdirs whether subdirectories should be monitored
// @param callback a function which will receive the kind of change
// plus the filename that changed. The change will be one of these:
//
// * FILE_ACTION_ADDED
// * FILE_ACTION_REMOVED
// * FILE_ACTION_MODIFIED
// * FILE_ACTION_RENAMED_OLD_NAME
// * FILE_ACTION_RENAMED_NEW_NAME
//
// @function watch_for_file_changes
def watch_for_file_changes (Str dir, Int how, Boolean subdirs, Value callback) {
  FileChangeParms *fc = (FileChangeParms*)malloc(sizeof(FileChangeParms));
  set_callback(fc,L,callback);
  fc->dir = dir;
  fc->how = how;
  fc->subdirs = subdirs;
  fc->hDir = CreateFile(dir,
    FILE_LIST_DIRECTORY,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    NULL,
    OPEN_ALWAYS,
    FILE_FLAG_BACKUP_SEMANTICS,
    NULL
    );
  if (fc->hDir == INVALID_HANDLE_VALUE) {
    return push_error(L);
  }
  fc->buffsize = 2048;
  fc->buff = (char*)malloc(fc->buffsize);
  _beginthread(&file_change_thread,THREAD_STACK_SIZE,fc);
  lua_pushboolean(L,1);
  return 1;
}

lua {
function winapi.execute(cmd)
   cmd = os.getenv('COMSPEC')..' /c '..cmd
   local P,f = winapi.spawn(cmd)
   if not P then return nil,f end
   local txt = f:read()
   local out = {}
   while txt do
     table.insert(out,txt)
     txt = f:read()
   end
   return P:wait():exit_code(),table.concat(out,'')
end
function winapi.match_name(text)
  return function(w) return tostring(w):match(text) end
end
function winapi.match_class(classname)
  return function(w) return w:get_class_name():match(classname) end
end
function winapi.find_window_ex(match)
  local res
  winapi.enum_windows(function(w)
    if match(w) then res = w end
  end)
  return res
end
function winapi.find_all_windows(match)
  local res = {}
  winapi.enum_windows(function(w)
    if match(w) then res[#res+1] = w end
  end)
  return res
end
function winapi.find_window_match(text)
  return winapi.find_window_ex(winapi.match_name(text))
end
}

/*** Constants.
The following constants are available:

 * SW_HIDE, (Window operations for Window.show)
 * SW_MAXIMIZE,
 * SW_MINIMIZE,
 * SW_SHOWNORMAL,
 * VK_BACK,
 * VK_TAB,
 * VK_RETURN,
 * VK_SPACE,
 * VK_PRIOR,
 * VK_NEXT,
 * VK_END,
 * VK_HOME,
 * VK_LEFT,
 * VK_UP,
 * VK_RIGHT,
 * VK_DOWN,
 * VK_INSERT,
 * VK_DELETE,
 * VK_ESCAPE,
 * VK_F1,
 * VK_F2,
 * VK_F3,
 * VK_F4,
 * VK_F5,
 * VK_F6,
 * VK_F7,
 * VK_F8,
 * VK_F9,
 * VK_F10,
 * VK_F11,
 * VK_F12,
 * FILE\_NOTIFY\_CHANGE\_FILE\_NAME  (these are input flags for watch\_for\_file\_changes)
 * FILE\_NOTIFY\_CHANGE\_DIR\_NAME
 * FILE\_NOTIFY\_CHANGE\_LAST\_WRITE
 * FILE\_ACTION\_ADDED     (these describe the change: first argument of callback)
 * FILE\_ACTION\_REMOVED
 * FILE\_ACTION\_MODIFIED
 * FILE\_ACTION\_RENAMED\_OLD\_NAME
 * FILE\_ACTION\_RENAMED\_NEW\_NAME

 @section constants
 */

 /// useful Windows API constants
 // @table constants

constants {
  SW_HIDE,
  SW_MAXIMIZE,
  SW_MINIMIZE,
  SW_SHOWNORMAL,
  VK_BACK,
  VK_TAB,
  VK_RETURN,
  VK_SPACE,
  VK_PRIOR,
  VK_NEXT,
  VK_END,
  VK_HOME,
  VK_LEFT,
  VK_UP,
  VK_RIGHT,
  VK_DOWN,
  VK_INSERT,
  VK_DELETE,
  VK_ESCAPE,
  VK_F1,
  VK_F2,
  VK_F3,
  VK_F4,
  VK_F5,
  VK_F6,
  VK_F7,
  VK_F8,
  VK_F9,
  VK_F10,
  VK_F11,
  VK_F12,
  FILE_NOTIFY_CHANGE_FILE_NAME,
  FILE_NOTIFY_CHANGE_DIR_NAME,
  FILE_NOTIFY_CHANGE_LAST_WRITE,
  FILE_ACTION_ADDED,
  FILE_ACTION_REMOVED,
  FILE_ACTION_MODIFIED,
  FILE_ACTION_RENAMED_OLD_NAME,
  FILE_ACTION_RENAMED_NEW_NAME
}

}
