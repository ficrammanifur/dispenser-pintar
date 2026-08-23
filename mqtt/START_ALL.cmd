@echo off
title ===== REFILLX SYSTEM =====
color 0A

echo.
echo ==========================================
echo     REFILLX MQTT SYSTEM
echo     ONE CLICK STARTUP
echo ==========================================
echo.

set PROJECT_DIR=C:\Users\muham\Project
set MOSQUITTO_DIR=C:\Program Files\mosquitto

:: ============================================
:: 0. KILL SEMUA PROSES YANG PAKAI PORT 1883
:: ============================================
echo [1/5] Membersihkan port 1883...
taskkill /f /im mosquitto.exe 2>nul
taskkill /f /im python.exe 2>nul
net stop mosquitto 2>nul
timeout /t 2 /nobreak > nul

:: ============================================
:: 1. START MOSQUITTO (dengan window baru)
:: ============================================
echo [2/5] Starting Mosquitto...
start "Mosquitto Broker" /min cmd /c "cd /d %MOSQUITTO_DIR% && mosquitto.exe -v -c mosquitto.conf"

:: Tunggu 5 detik biar Mosquitto benar-benar jalan
echo [3/5] Menunggu Mosquitto siap (5 detik)...
timeout /t 5 /nobreak > nul

:: Cek Mosquitto
netstat -an | findstr :1883 | findstr LISTENING > nul
if %errorlevel% == 0 (
    echo        ✅ Mosquitto running
) else (
    echo        ❌ Mosquitto GAGAL! Coba jalankan manual.
    echo        Buka terminal baru: cd C:\Program Files\mosquitto ^&^& mosquitto.exe -v -c mosquitto.conf
    pause
    exit
)

:: ============================================
:: 2. START GATEWAY
:: ============================================
echo [4/5] Starting Gateway...
start "MQTT Gateway" powershell -NoExit -Command "cd '%PROJECT_DIR%'; .\venv\Scripts\Activate.ps1; cd mqtt; Write-Host '[GATEWAY] Running...' -ForegroundColor Green; python gateway.py"

timeout /t 2 /nobreak > nul

:: ============================================
:: 3. START COMMANDER
:: ============================================
echo [5/5] Starting Commander...
start "MQTT Commander" powershell -NoExit -Command "cd '%PROJECT_DIR%'; .\venv\Scripts\Activate.ps1; cd mqtt; Write-Host '[COMMANDER] Ready!' -ForegroundColor Green; Write-Host ''; Write-Host 'Commands:' -ForegroundColor Yellow; Write-Host '  python send_command.py open 0' -ForegroundColor Cyan; Write-Host '  python send_command.py close 0' -ForegroundColor Cyan; Write-Host '  python send_command.py stop_all' -ForegroundColor Cyan"

echo.
echo ==========================================
echo   ✅ SEMUA SERVICE BERJALAN!
echo ==========================================
echo.
echo  TERMINALS OPENED:
echo   1. Mosquitto Broker    - (minimized)
echo   2. MQTT Gateway        - Bridge
echo   3. MQTT Commander      - Send Commands
echo.
echo  QUICK COMMANDS:
echo    python send_command.py open 0
echo    python send_command.py close 0
echo    python send_command.py stop_all
echo.
echo ==========================================
timeout /t 5 /nobreak > nul
exit