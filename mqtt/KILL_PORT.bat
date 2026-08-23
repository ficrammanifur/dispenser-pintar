@echo off
title KILL PORT 1883
color 0C

echo ==========================================
echo   MEMBERSIHKAN PORT 1883
echo ==========================================
echo.

echo [1/4] Menghentikan Service Mosquitto...
net stop mosquitto 2>nul
sc stop mosquitto 2>nul

echo [2/4] Mencari proses yang pakai port 1883...
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :1883 ^| findstr LISTENING') do (
    echo        Menemukan PID %%a
    echo        Membunuh PID %%a...
    taskkill /PID %%a /F 2>nul
)

echo [3/4] Mematikan Mosquitto...
taskkill /f /im mosquitto.exe 2>nul

echo [4/4] Mematikan Python...
taskkill /f /im python.exe 2>nul

echo.
echo ==========================================
echo   PORT 1883 SUDAH BERSIH!
echo ==========================================
echo.

netstat -an | findstr :1883
if %errorlevel% == 0 (
    echo ⚠️ Masih ada yang pakai port 1883!
    echo    Cek dengan: netstat -ano | findstr :1883
) else (
    echo ✅ Port 1883 sudah kosong!
)

echo.
pause