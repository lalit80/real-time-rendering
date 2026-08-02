# Real-Time Rendering

Hands-on graphics programs built while learning **Direct3D 11** and **OpenGL** (incl. OpenGL ES / WebGL), using both the **fixed-function** and **programmable** (shader) pipelines.

**Platforms:** Windows · Linux · macOS · Android · iOS · Web

## Topic path

Every platform follows the same route:

```
windowing → BlueScreen → projections → 2D/3D rotation → textures → sphere →
lighting → FBO → tessellation → geometry shaders → interleaved/indexed buffers →
model loading → CUDA/OpenCL interop
```

## Direct3D (`direct3d/`)

D3D11, HLSL compiled at runtime (`vs_5_0`/`ps_5_0`/`gs_5_0`), XNA Math, DXGI swap chain, DirectXTK `WICTextureLoader`, prebuilt `Sphere.lib` for sphere meshes.

| Area | Concepts |
|------|----------|
| Setup | Device/adapter info log, BlueScreen window, swap chain |
| Projections | Perspective, orthographic; triangle strip vs list |
| Rotation | 2D and 3D — triangles, rectangles, pyramids, cubes |
| Textures | Smiley, tweaked smiley, checkerboard on pyramid/cube |
| Sphere | Mesh loaded from `Sphere.lib`, drawn via `DrawIndexed` |
| Lighting | Diffuse → per-vertex → per-pixel → vertex/pixel toggle → 2 lights on pyramid → 3 lights on sphere → 24-sphere grid |
| Advanced | Tessellation shaders · geometry shaders · interleaved position/color/texcoord/normal buffers |

## OpenGL

| Platform | Toolkit | Pipeline |
|----------|---------|----------|
| Windows | Win32 + GLEW | Fixed-function **and** programmable |
| Linux | X11/GLX | Fixed-function **and** programmable |
| macOS | Cocoa/NSOpenGLView, core profile | Shader-based only |
| Android | Java + `GLSurfaceView` | OpenGL ES |
| iOS | Objective-C, `OpenGLES.framework` (ES 3.0) | Shader-based only |
| Web | HTML/JS, WebGL 2 | Shader-based only |

**Windowing** — Win32: message loop, fullscreen, icon, log file · X11: window, centered window, fullscreen · Cocoa: custom view, events, fullscreen · Android: template, fullscreen, landscape, events · Web: events, fullscreen.

**Fixed-function (Windows & Linux)** — GL driver info · ortho/perspective projections · 2D/3D rotation · matrix stacks (solar system, robotic arm) · depth buffering · textures (smiley, checkerboard) · lighting: diffuse, Gouraud, two lights, spot light, 24-sphere materials · special effects · Utah teapot.

**Programmable (Windows & Linux)** — GLSL projections · rotations · textures · sphere · full lighting progression · FBO render-to-texture · tessellation shaders · geometry shaders · interleaved buffers · indexed drawing · `.obj` model loading (Suzanne) · CUDA/OpenCL buffer sharing · graph paper · solar system · robotic arm.

**Shader-based (macOS / Android / iOS / Web)** — the same topic path: BlueScreen → projections → rotations → textures → sphere → lighting → FBO → interleaved → indexed drawing.

## Building

| Platform | Command |
|----------|---------|
| Windows | `Build.bat` in a Visual Studio Developer Command Prompt (`cl` + `rc` + `link`); GLEW for shader projects |
| Linux | `./build.sh` — gcc, `-lX11 -lGL`, plus `-lGLEW` for shaders |
| macOS | `./build.sh` — clang into a `.app` bundle (Cocoa/OpenGL frameworks) |
| Android | `./gradlew assembleDebug`, then `adb install -r app/build/outputs/apk/debug/app-debug.apk` |
| iOS | Open the `.xcodeproj` in Xcode |
| Web | Open `Canvas.html` in any WebGL 2 browser |

## Notes

- Build artifacts/IDE caches are gitignored; only source, shaders, textures, and prebuilt `.lib`/`.h` deps are tracked. The `assignments/` folder is local-only.
- Many projects include a screenshot/screen recording of the expected output.
