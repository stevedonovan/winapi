REM compiling for mingw against msvcrt
set LUA_DIR=C:\Users\steve\lua\lua-5.2.0\src
set CFLAGS=-c -O1 -DPSAPI_VERSION=1  -I"%LUA_DIR%"
gcc %CFLAGS% winapi.c
gcc %CFLAGS% wutils.c
gcc -Wl,-s -shared winapi.o wutils.o "%LUA_DIR%\lua52.dll" -lpsapi -lMpr -o winapi.dll