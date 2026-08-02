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

Windows and Linux split rendering into `FixedFunctionPipeline` (legacy immediate mode) and `ProgrammablePipeline` (GLSL). The programs follow a common path: windowing → BlueScreen → projections → 2D/3D rotation → textures → sphere → lighting → FBO → tessellation → geometry shaders → interleaved/indexed buffers → model loading → CUDA/OpenCL interop.

## Direct3D (`direct3d/`)

D3D11 with HLSL compiled at runtime (`vs_5_0`/`ps_5_0`/`gs_5_0`), XNA Math, DXGI swap chain, DirectXTK `WICTextureLoader`, and a prebuilt `Sphere.lib` for sphere meshes.

- Device/adapter info dump to a log
- Blue-screen window with a swap chain
- Perspective and orthographic projections (triangles, rectangles; triangle strip vs list)
- 2D and 3D rotation of triangles, rectangles, pyramids, and cubes
- Texture mapping — smiley, tweaked smiley, checkerboard, on pyramid/cube
- Sphere mesh via `DrawIndexed`
- Full lighting progression — diffuse, per-vertex, per-pixel, vertex/pixel toggle, two lights on a pyramid, three lights on a sphere, 24-sphere grid
- Tessellation shaders
- Geometry shaders
- Interleaved position/color/texcoord/normal vertex buffer

## OpenGL

- **Windows** — Win32 windowing (message loop, fullscreen, custom icon, log file). Fixed-function: GL driver info, ortho/perspective, 2D/3D rotation, matrix loading and matrix stacks (solar system, robotic arm), depth buffering, textures, lighting templates plus diffuse/Gouraud/two-light/spot-light/24-sphere material demos, special effects, Utah teapot. Programmable: GLSL ortho/perspective, rotations, textures, sphere, full lighting progression, FBO render-to-texture, tessellation shaders, geometry shaders, interleaved buffers, indexed drawing, `.obj` model loading (Suzanne), CUDA/OpenCL buffer sharing, graph paper, solar system, robotic arm.
- **Linux** — same fixed-function and programmable content as Windows, on X11.
- **macOS** — Cocoa windowing (events, fullscreen, custom view) + shader-based BlueScreen, projections, rotations, textures, sphere, lighting, FBO, tessellation, geometry shaders, interleaved buffers, indexed drawing.
- **Android** — windowing apps (template, fullscreen, landscape, events) + OpenGL ES: BlueScreen → textures → sphere → lighting → FBO, tessellation/geometry shaders, interleaved, indexed drawing (Gradle).
- **iOS** — Xcode projects, ES 3.0: BlueScreen → projections/rotations → textures → sphere → lighting → FBO → interleaved → indexed drawing.
- **Web** — HTML/JS WebGL 2: BlueScreen → projections/rotations → textures → sphere → lighting → interleaved → indexed drawing → FBO.

Lighting progresses uniformly everywhere: Diffuse → per-vertex → per-fragment/pixel → vertex/fragment toggle → two lights on a pyramid → three lights on a sphere → 24 spheres.

## Building

- **Windows** — run `Build.bat` from a Visual Studio Developer Command Prompt (`cl` + `rc` + `link`). Programmable projects link GLEW.
- **Linux** — `./build.sh` (gcc, `-lX11 -lGL`, plus `-lGLEW` for shaders).
- **macOS** — `./build.sh` (clang into a `.app` bundle, Cocoa/OpenGL frameworks).
- **Android** — `./gradlew assembleDebug`, then `adb install -r app/build/outputs/apk/debug/app-debug.apk`.
- **iOS** — open the `.xcodeproj` in Xcode.
- **Web** — open `Canvas.html` in any WebGL 2 browser.

## Notes

- Build artifacts/IDE caches are gitignored; only source, shaders, textures, and prebuilt `.lib`/`.h` deps are tracked. The `assignments/` folder is local-only.
- Many projects include a screenshot/screen recording of expected output.
