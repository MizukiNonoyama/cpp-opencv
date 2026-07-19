@echo off
setlocal EnableDelayedExpansion

rem ---------------------------------------------------------------------
rem Resets the CMake cache/configuration for this project.
rem   - Default: deletes CMakeCache.txt + CMakeFiles from each known
rem     build directory (build, cmake-build-debug, cmake-build-release),
rem     keeping any already-built binaries in place where possible.
rem   - /full   : deletes the entire build directories instead.
rem ---------------------------------------------------------------------

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

set "FULL_RESET=0"
if /i "%~1"=="/full" set "FULL_RESET=1"

set "DIRS=build cmake-build-debug cmake-build-release"

for %%D in (%DIRS%) do (
    set "TARGET=%PROJECT_DIR%\%%D"
    if exist "!TARGET!" (
        if "!FULL_RESET!"=="1" (
            echo [reset] Removing "!TARGET!" ...
            rmdir /s /q "!TARGET!"
        ) else (
            if exist "!TARGET!\CMakeCache.txt" (
                echo [reset] Deleting "!TARGET!\CMakeCache.txt" ...
                del /f /q "!TARGET!\CMakeCache.txt"
            )
            if exist "!TARGET!\CMakeFiles" (
                echo [reset] Removing "!TARGET!\CMakeFiles" ...
                rmdir /s /q "!TARGET!\CMakeFiles"
            )
        )
    )
)

echo [reset] Done.
exit /b 0