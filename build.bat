@echo off
cls

IF "%1"=="gcc" goto gcc

:msvc
cl *.c /I.\zlib-1.2.11 ws2_32.lib .\zlib-1.2.11\contrib\vstudio\vc14\x86\ZlibDllReleaseWithoutAsm\zlibwapi.lib /Feserver
goto end

:gcc
gcc *.c z.dll -lws2_32 -o server
goto end

:end
server
