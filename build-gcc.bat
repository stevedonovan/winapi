REM compiling for mingw against msvcrt
set LUA_DIR=C:\lua
gcc -c -O1 -DPSAPI_VERSION=1  -I"%LUA_DIR%\include"  winapi.c
gcc -Wl,-s -shared winapi.o "%LUA_DIR\lua51.dll" -lpsapi  -o winapi.dll