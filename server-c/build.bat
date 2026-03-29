@echo off

rem Define GCC path.
set gcc=C:\msys64\mingw32\bin\gcc

rem Parse parameters.
set target=
set flags=-Os -s
for %%a in (%*) do (
  if /i "%%a"=="-xp" (
    set target=-D_WIN32_WINNT=0x0501
  )
  if /i "%%a"=="-debug" (
    set flags=-g
  )
  if /i "%%a"=="-converter" (
    goto :converter
  )
)

rem Compile server executable.
set sources=^
  modules/libdb/libdb.c ^
  modules/libhttp/libhttp.c ^
  modules/libini/libini.c ^
  modules/libsocket/libsocket.c ^
  modules/libthread/libthread.c ^
  modules/libvector/libvector.c ^
  server/connection.c ^
  server/databases.c ^
  server/main.c ^
  server/loader.c ^
  server/options.c ^
  server/output.c ^
  server/models/ranking.c ^
  server/models/user.c ^
  server/services/service.c
set includes=-I./modules
set libraries=-L./modules/libdb -llmdb -static-libgcc -lws2_32
set output=server.exe

"%gcc%" %target% %flags% %sources% %includes% %libraries% -o "%output%"

rem Compile server hook.
set sources=^
  modules/libini/libini.c ^
  modules/libvector/libvector.c ^
  hook/hook.c ^
  hook/main.c ^
  hook/options.c
set includes=-I./modules
set libraries=-L./modules/minhook -lminhook -static-libgcc -lwininet -shared
set output=server.dll

"%gcc%" %target% %flags% %sources% %includes% %libraries% -o "%output%"

goto :EOF

:converter
rem Compile version converter tool.
set sources=^
  modules/libdb/libdb.c ^
  modules/libvector/libvector.c ^
  converter/converter.c ^
  converter/databases.c ^
  converter/main.c
set includes=-I./modules
set libraries=-L./modules/libdb -llmdb -static-libgcc
set output=converter.exe

"%gcc%" %target% %flags% %sources% %includes% %libraries% -o "%output%"