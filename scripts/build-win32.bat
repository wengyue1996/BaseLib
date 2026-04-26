@echo off
echo ========================================
echo   BaseLib Build Script (Win32)
echo ========================================
echo.

powershell -ExecutionPolicy Bypass -File "%~dp0build-package.ps1" -Platform Win32 %*

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Build completed successfully!
pause
