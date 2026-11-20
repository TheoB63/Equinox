@echo off

pushd ..\..
MSBuild Equinox.sln /p:Configuration=Debug /p:Platform=x64
popd
pause