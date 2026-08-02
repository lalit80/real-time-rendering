# Real-Time Rendering

A collection of real-time rendering programs built while learning OpenGL and Direct3D, covering the fixed-function pipeline and the programmable pipeline across multiple platforms.

## Repository layout

```
direct3d/   Direct3D 11 programs (DirectX)
opengl/     OpenGL programs, grouped by platform:
  android/    Android (OpenGL ES via Gradle)
  ios/        iOS (Xcode / Metal-adjacent OpenGL)
  linux/      Linux (X11)
  mac/        macOS (Cocoa)
  web/        WebGL (HTML/JS)
  windows/    Windows (Win32)
```

Topic folders follow the `NN-Name` convention and are PascalCased:

- `01-*` `02-*` – windowing basics, BlueScreen
- `03-*` – perspective, orthographic projection
- `05-2DRotation` / `06-3DRotation` – rotations
- `07-Texture` – texture mapping
- `09-Light` – lighting (diffuse, per-vertex/per-fragment)
- `10-Tessellation`, `11-GeometryShader`, `13-Interleaved`, `14-FBO`, etc.

Within `opengl/{linux,windows}/02-OpenGL`:

- `01-FixedFunctionPipeline` – legacy immediate-mode-style OpenGL
- `02-ProgrammablePipeline` – shader-based OpenGL (GLSL)

## Building

### Windows – Direct3D and OpenGL

Each project contains a `Build.bat`:

```bat
cl.exe /c /EHsc d3d.cpp
rc.exe d3d.rc
link.exe d3d.obj d3d.res user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
```

Run it from a **Developer Command Prompt for Visual Studio** (so `cl`, `rc`, and `link` are on `PATH`).

### Linux

Each project contains a `build.sh`:

```sh
gcc -c -o OGL.o OGL.c
gcc -o OGL OGL.o -lX11 -lGL
./OGL
```

Shader-based programs additionally link `-lGLEW`.

### macOS

Each project contains a `build.sh` that compiles with `clang` against the Cocoa/Foundation frameworks and produces a `.app` bundle:

```sh
clang -o window.app/Contents/MacOS/window window.m -framework Foundation -framework Cocoa
```

### Android

Projects are standard Gradle apps under `opengl/android`. Build with the wrapper:

```sh
cd opengl/android/OpenGL-ES/01-BlueScreen
./gradlew assembleDebug
```

### Web

`opengl/web` projects are plain HTML/JS (WebGL). Open the `Canvas.html` file in a browser.

## Notes

- Build artifacts and IDE caches are ignored via `.gitignore`; only source, shaders, textures, and prebuilt `.lib` dependencies are tracked.
- Numbering is not strictly sequential across platforms — each platform's folder is self-contained.
