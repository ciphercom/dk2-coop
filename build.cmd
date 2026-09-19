@echo off
setlocal

rem Build, test, and install the standalone fork; optionally create the distributable ZIP.
set "PACKAGE_COOP="
set "BUILD_CONFIG="
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--package" (
  set "PACKAGE_COOP=1"
) else if /i "%~1"=="--release" (
  if /i "%BUILD_CONFIG%"=="Debug" goto :conflicting_config
  set "BUILD_CONFIG=Release"
) else if /i "%~1"=="--debug" (
  if /i "%BUILD_CONFIG%"=="Release" goto :conflicting_config
  set "BUILD_CONFIG=Debug"
) else (
  echo Usage: build.cmd [--release ^| --debug] [--package] 1>&2
  exit /b 2
)
shift /1
goto :parse_args
:args_done
rem Distribution defaults to Release; local development stays Debug.
if not defined BUILD_CONFIG (
  if defined PACKAGE_COOP (set "BUILD_CONFIG=Release") else (set "BUILD_CONFIG=Debug")
)
set "BUILD_FLAVOR=debug"
set "INSTALL_DIR=install"
if /i "%BUILD_CONFIG%"=="Release" (
  set "BUILD_FLAVOR=release"
  set "INSTALL_DIR=install-release"
)
goto :build_start

:conflicting_config
echo ERROR: --debug and --release cannot be combined. 1>&2
exit /b 2

:build_start

rem Keep the supported local build independent of the caller's current directory.
pushd "%~dp0" || exit /b 1

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: vswhere.exe was not found. Install Visual Studio Installer and Visual Studio 2026 or 2022. 1>&2
  goto :failure
)

set "VS_INSTALL="
set "VS_GENERATION=2026"
set "VSWHERE_RESULT=%~dp0.vswhere-%RANDOM%-%RANDOM%.tmp"
"%VSWHERE%" -latest -products * -version "[18.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSWHERE_RESULT%"
if errorlevel 1 (
  del /q "%VSWHERE_RESULT%" >nul 2>&1
  echo ERROR: vswhere.exe failed while locating Visual Studio 2026. 1>&2
  goto :failure
)
set /p VS_INSTALL=<"%VSWHERE_RESULT%"
rem GitHub's Windows runner uses VS2022; local VS2026 remains preferred.
if not defined VS_INSTALL (
  set "VS_GENERATION=2022"
  "%VSWHERE%" -latest -products * -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSWHERE_RESULT%"
  if errorlevel 1 goto :failure
  set /p VS_INSTALL=<"%VSWHERE_RESULT%"
)
del /q "%VSWHERE_RESULT%"
set "BUILD_PRESET=vs%VS_GENERATION%-%BUILD_FLAVOR%"
if not defined VS_INSTALL (
  echo ERROR: Visual Studio 2026 or 2022 with the x86 C++ build tools was not found. 1>&2
  goto :failure
)
if not exist "%VS_INSTALL%\VC\Auxiliary\Build\vcvars32.bat" (
  echo ERROR: The Visual Studio x86 C++ toolchain is incomplete. 1>&2
  goto :failure
)

set "CMAKE_EXE="
for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
if not defined CMAKE_EXE set "CMAKE_EXE=%VS_INSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" (
  echo ERROR: cmake.exe is not on PATH and Visual Studio's bundled CMake was not found. 1>&2
  goto :failure
)

"%CMAKE_EXE%" --preset %BUILD_PRESET%
if errorlevel 1 goto :failure

"%CMAKE_EXE%" --build --preset %BUILD_PRESET%
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\instance_mutex_name_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\coop_campaign_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\shared_dig_overlay_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\coop_campaign_progress_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\coop_campaign_init_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\scripted_camera_timeline_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\network_possession_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\command_line_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\session_discovery_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\health_flower_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\network_hands_tests.exe"
if errorlevel 1 goto :failure

".venv\Scripts\python.exe" "tools\tests\network_hands_native_tests.py"
if errorlevel 1 goto :failure

".venv\Scripts\python.exe" "tools\tests\network_possession_native_tests.py"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\network_gem_ending_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\network_gem_ending_ui_tests.exe"
if errorlevel 1 goto :failure

"build\%BUILD_PRESET%\src\%BUILD_CONFIG%\input_cursor_tests.exe"
if errorlevel 1 goto :failure

".venv\Scripts\python.exe" "tools\package_coop_tests.py"
if errorlevel 1 goto :failure

".venv\Scripts\python.exe" "tools\check_release_tests.py"
if errorlevel 1 goto :failure

"%CMAKE_EXE%" --build --preset %BUILD_PRESET%-install
if errorlevel 1 goto :failure

for %%F in ("%INSTALL_DIR%\PATCH.dll" "%INSTALL_DIR%\flame\Flame.dll" "%INSTALL_DIR%\flame\DKII.dll") do (
  if not exist "%%~F" (
    echo ERROR: Required build artifact was not installed: %%~F 1>&2
    goto :failure
  )
)

if defined PACKAGE_COOP (
  ".venv\Scripts\python.exe" "tools\package_coop.py" --configuration %BUILD_CONFIG% --build-directory "build\%BUILD_PRESET%"
  if errorlevel 1 goto :failure
  popd
  exit /b 0
)

popd
exit /b 0

:failure
set "BUILD_EXIT_CODE=%ERRORLEVEL%"
if "%BUILD_EXIT_CODE%"=="0" set "BUILD_EXIT_CODE=1"
popd
exit /b %BUILD_EXIT_CODE%
