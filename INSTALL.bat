@echo off
title INSTALL DEPENDENCIES
color 0A

echo.
echo ==========================================
echo     INSTALLING DEPENDENCIES
echo ==========================================
echo.

cd /d C:\Users\muham\Project

echo Activating virtual environment...
.\venv\Scripts\Activate.ps1

echo Installing requirements...
pip install paho-mqtt

echo.
echo ✅ Installation complete!
pause