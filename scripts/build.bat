@echo off
echo ========================================
echo   BaseLib Build Script
echo ========================================
echo.

powershell -ExecutionPolicy Bypass -File "%~dp0build-package.ps1" %*

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Build completed successfully!
pause
