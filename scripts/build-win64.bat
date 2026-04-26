@echo off
echo ========================================
echo   BaseLib Build Script (Win64)
echo ========================================
echo.

powershell -ExecutionPolicy Bypass -File "%~dp0build-package.ps1" -Platform x64 %*

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Build completed successfully!
pause
