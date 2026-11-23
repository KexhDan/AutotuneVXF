# AutoVox Tune (VST3, JUCE)

Plugin VST3 de **corrección de pitch para voces** al estilo Auto-Tune, con selección de
escala, velocidad de salto de nota y humanización del efecto.

> Es una implementación propia inspirada en el funcionamiento de los afinadores vocales,
> no un clon de Auto-Tune ni un producto de Anthem Audio.

## Características

- **Formato:** VST3 (mono o estéreo; la detección se hace en mono y el mismo ajuste se
  aplica a todos los canales).
- **Framework:** JUCE 8.0.4, descargado automáticamente con `FetchContent` (igual que tu
  proyecto `FET1176Compressor`).
- **RAIZ:** 12 notas (C ... B).
- **ESCALA:** 13 opciones — Cromática, Mayor, Menor natural, Menor armónica, Menor
  melódica, Dorio, Frigio, Lidio, Mixolidio, Locrio, Pentatónica mayor, Pentatónica
  menor y Blues.
- **VELOCIDAD DE SALTO** (`Retune Speed`): 0 = deslizamiento natural (~200 ms),
  100 = afinación instantánea (~2 ms, el típico efecto "robótico" de Auto-Tune).
- **HUMANIZACIÓN** (`Humanize`): añade vibrato (~5 Hz, hasta ±14 cents) y una deriva
  aleatoria lenta, para que la nota corregida no quede clavada y suene más natural.
- **MEZCLA** (`Mix`): dry/wet. El seco se retrasa lo mismo que el húmedo para evitar
  comb-filtering.
- **A4 (Hz):** referencia de afinación, de 430 a 450 Hz (440 por defecto).
- **TRANSPOSE:** desplazamiento de **-12 a +12 semitonos** (una octava abajo/arriba), con
  valor inicial **0** justo en el centro y pasos de 1 semitono. Se suma a la nota resultante
  de la escala: con **0 st la escala no se modifica**.
- **FORMANT:** desplazamiento de los formantes vocales de **-12 a +12 semitonos** (0 = neutro),
  para cambiar el carácter de la voz (más grave/amplia o más aguda/pequeña) sin tocar el pitch.
- **Visualizador:** nota detectada, desviación en cents, nota objetivo, corrección
  aplicada y medidor de cents en tiempo real.

## Estructura

```
AutoVoxTune/
  CMakeLists.txt
  Source/
    PluginProcessor.h/.cpp   Parametros, bucle de audio, deteccion y control
    PluginEditor.h/.cpp      Interfaz y visualizador
    PitchDetector.h/.cpp     Detector YIN
    PitchShifter.h/.cpp      Desplazador de pitch por crossfade
    FormantFilter.h/.cpp     Banco de filtros de formantes (F1/F2/F3)
    MusicalScale.h/.cpp      Escalas, notas y conversiones MIDI <-> Hz
```

## Cómo funciona (resumen técnico)

1. **Detección:** algoritmo **YIN** sobre ventanas de 2048 muestras solapadas cada 512
   muestras (~11.6 ms a 44.1 kHz). La señal se diezma por 2 para abaratar el coste.
   Devuelve frecuencia fundamental y un índice de "claridad"; si no hay voz suficiente,
   la corrección se desactiva suavemente.
2. **Cuantización:** la frecuencia se convierte a MIDI, se redondea a la nota permitida
   más cercana dentro de la escala elegida y se vuelve a convertir a Hz.
3. **Velocidad de salto:** la corrección (en cents) se suaviza con un filtro de un polo
   cuya constante de tiempo se deriva del parámetro `Retune Speed`. Eso controla lo
   rápido que el plugin "salta" de una nota corregida a la siguiente.
4. **Desplazamiento:** dos cabezas de lectura separadas medio buffer, con crossfade en
   ventana Hann, leyendo el buffer circular a `ratio = 2^(cents/1200)` veces la velocidad
   de escritura. Latencia nominal: 1024 muestras (~23 ms a 44.1 kHz).
5. **Humanización:** un LFO senoidal (~5 Hz) y una deriva pseudoaleatoria se suman
   **después** del suavizado de velocidad, de modo que el vibrato sigue audible incluso
   con `Velocidad` baja, y se funde suavemente al entrar/salir de la voz.
6. **Transpose:** se añade a la nota objetivo **después** de la cuantización de escala
   (`nota_escala + transpose_st`). Con el valor inicial 0 st, la escala actúa sola.
7. **Formantes:** sobre la señal ya afinada se aplica un banco de filtros peaking que
   realza F1/F2/F3 en `base * 2^(formant/12)` y atenúa su posición original, más un
   estante agudo suave. Solo afecta al canal húmedo, así que `Mix` sigue siendo un
   dry/wet limpio.

## Requisitos para compilar

- **CMake** >= 3.22
- **Compilador C++17** (Visual Studio 2022 con "Desktop development with C++", MSVC Build
  Tools, Clang o GCC)
- **Git** (CMake descarga JUCE 8.0.4)
- Conexión a Internet la primera vez

## Compilar (Windows, Visual Studio)

Doble clic en `build.bat`, o manualmente:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Compilar con NMake (como el otro proyecto) o Ninja

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

El VST3 queda en:

```
build/AutoVoxTune_artefacts/Release/VST3/AutoVox Tune.vst3
```

Con `COPY_PLUGIN_AFTER_BUILD TRUE` también se copia a la carpeta de plugins VST3 del
sistema (puede requerir permisos de administrador; si falla, copia manualmente el `.vst3`
a `C:\Program Files\Common Files\VST3`).

Después de copiarlo, reinicia tu DAW y haz un escaneo de plugins.

## Consejos de uso

- Para el efecto **clásico "hard tune"**: escala acorde con la canción, `Velocidad` cerca
  de 100 y `Humanización` baja (0-15).
- Para un resultado **natural**: `Velocidad` entre 15 y 40 y `Humanización` entre 25 y 50.
- Canta con una sola nota a la vez: es un afinador **monofónico** (como la mayoría de los
  Auto-Tune en modo automático). No corrige acordes.
- Si notas "metallic/artefactos" con saltos grandes de nota, baja la velocidad: el
  desplazador en dominio del tiempo es más limpio para correcciones pequeñas.
- `Transpose` desplaza la nota objetivo en semitonos enteros (una octava abajo/arriba).
  Con 0 se respeta solo la escala; con +12 la misma nota suena una octava más alto.
- `Formant` no cambia la nota: mueve el énfasis de los formantes. Valores negativos
  dan una voz más oscura/grande; positivos, más brillante/pequeña.

## Licencia

Código del proyecto: uso libre para aprendizaje y modificación. JUCE se descarga desde su
repositorio oficial y está sujeto a su propia licencia (ver JUCE).
