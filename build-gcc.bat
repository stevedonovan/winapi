REM compiling for mingw against msvcrt
# set LUA_DIR=D:\dev\lua\luajit-2.0\src
set LUA_INCLUDE=c:\users\steve\luadist\include
set LUA_LIB=c:\users\steve\luadist\lib\liblua51.dll.a
set CFLAGS=-c -O1 -DPSAPI_VERSION=1  -I"%LUA_INCLUDE%"
gcc %CFLAGS% winapi.c
gcc %CFLAGS% wutils.c
gcc -Wl,-s -shared winapi.o wutils.o "%LUA_LIB%" -lpsapi -lMpr -o winapi.dll