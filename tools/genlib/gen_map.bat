@echo off
setlocal

echo extracting export symbols "%~2" -^> "%~3"
"%~1" /exports "%~2" > "%~3.dumpbin.tmp"
if errorlevel 1 (
  del /q "%~3.dumpbin.tmp" >nul 2>&1
  exit /b 1
)

> "%~3" echo LIBRARY %~4
>> "%~3" echo EXPORTS
for /f "usebackq skip=19 tokens=4" %%A in ("%~3.dumpbin.tmp") do >> "%~3" echo %%A
del /q "%~3.dumpbin.tmp"
exit /b 0

