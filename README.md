# Real-Time Rendering

Hands-on graphics programs built while learning **Direct3D 11** and **OpenGL** (incl. OpenGL ES / WebGL), on Windows, Linux, macOS, Android, iOS, and the browser — using both the fixed-function and programmable (shader) pipelines.

```
direct3d/                  Direct3D 11 (Windows)
opengl/
  windows/                 Win32 + GLEW
  linux/                   X11/GLX
  mac/                     Cocoa/NSOpenGLView (core profile)
  android/                 Java + GLSurfaceView (OpenGL ES)
  ios/                     Objective-C + OpenGLES.framework
  web/                     WebGL 2 (HTML/JS)
```

Windows and Linux split rendering into `01-FixedFunctionPipeline` (legacy immediate mode) and `02-ProgrammablePipeline` (GLSL). Topics are numbered `NN-Name` and follow a common path: windowing → BlueScreen → projections → 2D/3D rotation → textures → sphere → lighting → FBO → tessellation → geometry shaders → interleaved/indexed buffers → model loading → CUDA/OpenCL interop.

## Direct3D (`direct3d/`)

D3D11 with HLSL compiled at runtime (`vs_5_0`/`ps_5_0`/`gs_5_0`), XNA Math, DXGI swap chain, DirectXTK `WICTextureLoader`, and a prebuilt `Sphere.lib` for sphere meshes.

`01-PrintDXInfo` · `02-BlueScreen` · `03-Perspective` · `04-Orthographic` · `05-2DRotation` / `05-3DRotation` · `06-Texture` · `07-Sphere` · `08-Light` · `09-Tessellation` · `10-Geometry` / `11-GeometryNew` · `12-Interleaved`

## OpenGL

- **Windows** — `01-Windowing` (7 apps) + FFP (`01-…16`, incl. GLInfo, matrices, textures, lighting, teapot) + programmable (`01-…19`, incl. FBO, tessellation, geometry shaders, `.obj` model loading, CUDA/OpenCL, solar system, robotic arm).
- **Linux** — mirrors Windows (`01-…16` FFP, `01-…15` programmable).
- **macOS** — `01-Windowing` (7 apps) + shader-based `01-…13`.
- **Android** — windowing apps `01-…06` + `OpenGL-ES` `01-…14` (Gradle).
- **iOS** — Xcode projects, ES 3.0: `01-…11`.
- **Web** — HTML/JS WebGL 2: `01-…12`.

Lighting progresses uniformly everywhere: Diffuse → per-vertex → per-fragment/pixel → PV/PF toggle → 2 lights on pyramid → 3 lights on sphere → 24 spheres.

## Building

- **Windows** — run `Build.bat` from a Visual Studio Developer Command Prompt (`cl` + `rc` + `link`). Programmable projects link GLEW.
- **Linux** — `./build.sh` (gcc, `-lX11 -lGL`, plus `-lGLEW` for shaders).
- **macOS** — `./build.sh` (clang into a `.app` bundle, Cocoa/OpenGL frameworks).
- **Android** — `./gradlew assembleDebug`, then `adb install -r app/build/outputs/apk/debug/app-debug.apk`.
- **iOS** — open the `.xcodeproj` in Xcode.
- **Web** — open `Canvas.html` in any WebGL 2 browser.

## Notes

- Build artifacts/IDE caches are gitignored; only source, shaders, textures, and prebuilt `.lib`/`.h` deps are tracked. The `assignments/` folder is local-only.
- Topic numbering isn't consistent across platforms; each platform folder is self-contained.
- Many projects include a screenshot/screen recording of expected output.
