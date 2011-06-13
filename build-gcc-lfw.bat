REM building with mingw for LfW
set LUA_DIR=C:\Program Files\Lua\5.1
set CFLAGS=-Os -DPSAPI_VERSION=1  -I"%LUA_DIR%\include"
gcc -c %CFLAGS% winapi.c
gcc -c %CFLAGS% wutils.c
gcc -Wl,-s -shared winapi.o wutils.o -L"%LUA_DIR%/lib"  -lpsapi -lMpr -llua5.1 -lmsvcr80  -o winapi.dll