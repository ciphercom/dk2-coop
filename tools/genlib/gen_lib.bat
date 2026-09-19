@echo off
setlocal

echo build lib from symbols "%~2" -^> "%~3"
"%~1" /def:"%~2" /out:"%~3" /machine:x86
exit /b %ERRORLEVEL%
