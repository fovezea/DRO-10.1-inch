@echo off
REM Windows batch script wrapper for fix_manifest_paths.py
REM Usage: fix_manifest_paths.bat [--backup] [--dry-run] [--verbose]

echo ESP-IDF Manifest Path Fixer
echo ===========================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python and try again
    pause
    exit /b 1
)

REM Run the Python script with all arguments
python fix_manifest_paths.py %*

REM Check if the script ran successfully
if errorlevel 1 (
    echo.
    echo ERROR: The script encountered an error
    pause
    exit /b 1
)

echo.
echo Script completed successfully!
echo.
echo Next steps:
echo 1. Run 'idf.py reconfigure' to verify the fix
echo 2. Test your build with 'idf.py build'
echo.
pause 