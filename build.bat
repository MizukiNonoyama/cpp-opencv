@echo off
setlocal EnableDelayedExpansion

rem START CLAUDE CODE
rem ---------------------------------------------------------------------
rem Reproduces, as a script, everything done interactively to set up and
rem verify this project:
rem   1. Ensure a real Python is available (install via winget if missing).
rem      NOTE: Windows always has a "python.exe" on PATH even when Python
rem      isn't installed - it's a Microsoft Store app-execution-alias stub
rem      that runs but prints no real version and is useless for pip. So
rem      we don't just check "where python", we actually run it and check
rem      the output/exit code.
rem   2. Ensure Conan is available (install via pip if missing).
rem   3. Locate cmake.exe (PATH, else CLion / VS Build Tools bundle).
rem   4. Configure with CMAKE_PROJECT_TOP_LEVEL_INCLUDES pointing at
rem      cmake\ConanBootstrap.cmake, which auto-runs `conan install` for
rem      any non-OpenCV dependency (fmt) the first time it's needed.
rem   5. Build (Release).
rem   6. Run the resulting exe, forwarding any command-line arguments
rem      (e.g. an image path) to it.
rem ---------------------------------------------------------------------

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"
set "BUILD_DIR=%PROJECT_DIR%\build"

echo ============================================================
echo  1. Python
echo ============================================================

rem Prepend any already-installed real Python (python.org / winget) found
rem under the user profile, so it wins over the WindowsApps stub on PATH.
for /d %%D in ("%LOCALAPPDATA%\Programs\Python\Python3*") do (
    if exist "%%D\python.exe" set "PATH=%%D;%%D\Scripts;!PATH!"
)

call :CheckPython
if "!PYTHON_OK!"=="0" (
    echo [build] Python not found / not runnable. Installing via winget ...
    winget install --id Python.Python.3.12 -e --silent --accept-package-agreements --accept-source-agreements
    if errorlevel 1 (
        echo [build] ERROR: winget install of Python failed.
        exit /b 1
    )
    for /d %%D in ("%LOCALAPPDATA%\Programs\Python\Python3*") do (
        if exist "%%D\python.exe" set "PATH=%%D;%%D\Scripts;!PATH!"
    )
    call :CheckPython
)
if "!PYTHON_OK!"=="0" (
    echo [build] ERROR: python still not runnable after install. Open a new shell and re-run this script.
    exit /b 1
)
python --version

echo ============================================================
echo  2. Conan
echo ============================================================
conan --version >nul 2>nul
if errorlevel 1 (
    echo [build] Conan not found. Installing via pip ...
    python -m pip install --quiet --upgrade pip conan
    if errorlevel 1 (
        echo [build] ERROR: pip install of conan failed.
        exit /b 1
    )
    for /f "delims=" %%D in ('python -c "import sysconfig; print(sysconfig.get_path('scripts'))"') do set "PATH=%%D;!PATH!"
)

conan --version
if errorlevel 1 (
    echo [build] ERROR: conan still not runnable after install. Open a new shell and re-run this script.
    exit /b 1
)
conan profile detect --exist-ok

echo ============================================================
echo  3. Locate cmake.exe
echo ============================================================
set "CMAKE_EXE="
where cmake >nul 2>nul
if not errorlevel 1 (
    set "CMAKE_EXE=cmake"
) else (
    for %%C in (
        "C:\Program Files\JetBrains\CLion 2026.1.4\bin\cmake\win\x64\bin\cmake.exe"
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    ) do (
        if not defined CMAKE_EXE if exist %%C set "CMAKE_EXE=%%~C"
    )
)
if not defined CMAKE_EXE (
    echo [build] ERROR: cmake.exe not found. Install CMake, or CLion / Visual Studio which bundle it.
    exit /b 1
)
echo [build] Using cmake: !CMAKE_EXE!

echo ============================================================
echo  4. Configure - triggers the semi-automatic Conan bootstrap
echo ============================================================
"!CMAKE_EXE!" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="%PROJECT_DIR%\cmake\ConanBootstrap.cmake"
if errorlevel 1 (
    echo [build] ERROR: CMake configure failed.
    exit /b 1
)

echo ============================================================
echo  5. Build
echo ============================================================
"!CMAKE_EXE!" --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [build] ERROR: build failed.
    exit /b 1
)

echo ============================================================
echo  6. Run
echo ============================================================
"%BUILD_DIR%\Release\clion_cmake.exe" %*
exit /b 0

:CheckPython
rem Sets PYTHON_OK=1 if "python --version" prints a real "Python 3.x" line,
rem 0 otherwise (also catches the Microsoft Store stub, which prints
rem nothing useful and exits non-zero).
set "PYTHON_OK=0"
for /f "delims=" %%V in ('python --version 2^>nul') do (
    echo %%V | findstr /b /r "Python [0-9]" >nul && set "PYTHON_OK=1"
)
exit /b 0

rem END CLAUDE CODE
