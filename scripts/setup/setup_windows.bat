@echo off

pushd ..\..
extern\premake\windows\premake5.exe --file=premake5.lua vs2026
popd
pause