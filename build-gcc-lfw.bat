REM building with mingw for LfW
set LUA_DIR=C:\Program Files\Lua\5.1
gcc -c -O1 -DPSAPI_VERSION=1  -I"%LUA_DIR%\include"  winapi.c
gcc -Wl,-s -shared winapi.o  -L"%LUA_DIR%/lib"  -lpsapi -llua5.1 -lmsvcr80  -o winapi.dll