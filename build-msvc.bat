set LUA_DIR=C:\Program Files\Lua\5.1
cl /nologo -c /O1 /DPSAPI_VERSION=1  /I"%LUA_DIR%\include"  winapi.c
link /nologo winapi.obj  /EXPORT:luaopen_winapi  /LIBPATH:"%LUA_DIR%\lib" msvcrt.lib kernel32.lib user32.lib psapi.lib advapi32.lib shell32.lib lua5.1.lib  /DLL /OUT:winapi.dll
