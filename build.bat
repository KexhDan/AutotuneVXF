@echo off
setlocal

echo === Configurando AutoVox Tune (Visual Studio 2022, x64) ===
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1

echo.
echo === Compilando Release ===
cmake --build build --config Release
if errorlevel 1 exit /b 1

echo.
echo === Listo ===
echo El VST3 esta en:
echo   build\AutoVoxTune_artefacts\Release\VST3\AutoVox Tune.vst3
echo.
echo Copialo a C:\Program Files\Common Files\VST3 y reinicia tu DAW.
endlocal
