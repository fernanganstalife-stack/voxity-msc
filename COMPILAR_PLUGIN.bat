@echo off
echo Configurando el proyecto MSC Voxity...
cd /d "%~dp0"
cmake -B build -S . -A x64

echo.
echo Compilando el plugin en modo Release...
cmake --build build --config Release

echo.
echo ¡Proceso finalizado! Revisa la carpeta "build" para encontrar tu VST3.
pause