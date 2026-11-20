@echo off

pushd ..\..
MSBuild Equinox.slnx /p:Configuration=Debug /p:Platform=x64
popd
pause