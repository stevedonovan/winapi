REM compiling for mingw against msvcrt
set LUA_DIR=C:\Users\steve\luadist
set CFLAGS=-c -O1 -DPSAPI_VERSION=1  -I"%LUA_DIR%\include"
gcc %CFLAGS% winapi.c
gcc %CFLAGS% wutils.c
gcc -Wl,-s -shared winapi.o wutils.o "%LUA_DIR%\bin\liblua51.dll" -lpsapi -lMpr -o winapi.dll