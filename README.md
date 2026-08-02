# Real-Time Rendering

A hands-on collection of real-time rendering programs built while learning graphics programming with **Direct3D 11** and **OpenGL** (including **OpenGL ES** and **WebGL**). Every topic is implemented across multiple platforms — Windows, Linux, macOS, Android, iOS, and the browser — using both the legacy **fixed-function pipeline** and the **programmable (shader) pipeline**.

The progression follows a classic RTR syllabus: get a window up → clear it → draw primitives → apply projections → rotate → texture-map → light → render off-screen → tessellate → geometry shaders → interleaved/indexed buffers → model loading → heterogeneous (CUDA/OpenCL) programming.

## Repository layout

```
direct3d/                    Direct3D 11 programs (DirectX, Windows)
opengl/
  windows/                   Win32 (GLEW + GLSL for the programmable pipeline)
  linux/                     X11 / Xlib + GLX
  mac/                       Cocoa / NSOpenGLView (all shader-based, core profile)
  android/                   Android (Java + GLSurfaceView, OpenGL ES via Gradle)
  ios/                       iOS (Objective-C + OpenGLES.framework, Xcode projects)
  web/                       WebGL 2 (plain HTML/JS)
```

Each platform folder is split into a **windowing stage** (`01-Windowing`, etc.) followed by the **rendering stage** (`02-OpenGL`, `02-WebGL`, or `OpenGL-ES`). Windows and Linux additionally split the rendering stage into:

- `01-FixedFunctionPipeline` — legacy immediate-mode style (`glBegin`/`glEnd`, `glMatrixMode`, etc.)
- `02-ProgrammablePipeline` — modern shader-based OpenGL (GLSL, VBOs/VAOs, shaders)

Topic folders follow the `NN-Name` convention and are PascalCased. Numbering is not strictly sequential across platforms — each platform folder is self-contained.

## Topics covered

| # | Topic | What it demonstrates |
|---|-------|----------------------|
| `01-*` | Windowing / BlueScreen | Window creation + message/event loop, fullscreen toggle, clearing the screen |
| `02-*` | Primitives | Triangle / rectangle (polygon, triangle strip/list) |
| `03-*` / `04-*` | Projections | Perspective and orthographic projection matrices |
| `05-*` / `06-*` | Rotation | 2D rotation and 3D rotation (pyramid, cube) |
| `07-*` / `08-*` | Texture | Texture mapping (pyramid, cube, smiley, checkerboard) |
| `Sphere` | Sphere | Sphere mesh from a prebuilt `Sphere.lib` / generated geometry |
| `Light` | Lighting | Diffuse, per-vertex / per-fragment (pixel) shading, multiple lights, 24-sphere material grid |
| `FBO` | Off-screen rendering | Render-to-texture via framebuffer objects |
| `Tessellation` | Tessellation shaders | Hull + domain shaders (GPU tessellation) |
| `Geometry` | Geometry shaders | Geometry shaders (point → triangle expansion) |
| `Interleaved` | Interleaved buffers | Position/color/texcoord/normal packed into one VBO |
| `IndexedDrawing` | Index buffers | `glDrawElements` / `DrawIndexed` |
| `ModelLoading` | Model loading | `.obj` (Suzanne) loaders, 5 sessions |
| `Heterogeneous…` | GPGPU interop | OpenGL + CUDA and OpenGL + OpenCL sharing buffers |

## Direct3D 11 (`direct3d/`)

Built with the native Direct3D 11 API:

- **HLSL shaders compiled at runtime** via `D3DCompile` — shader model 5.0 (`vs_5_0`, `ps_5_0`, `gs_5_0`, and hull/domain for tessellation)
- **XNA Math (`xnamath`)** for `XMMATRIX` math
- **DXGI swap chain** (`IDXGISwapChain`) and `ID3D11RenderTargetView`
- Textures loaded with **DirectXTK `WICTextureLoader`**
- Sphere geometry from the prebuilt **`Sphere.h` / `Sphere.lib`** (see `How-To-Use-Sphere-DLL-In-D3D11-pSysMem.txt`)
- Vertex data reference arrays in `DirectX_3D_PCNT.cpp.txt` (position/color/texcoord/normal for pyramid & cube)

| Folder | Content |
|--------|---------|
| `01-PrintDXInfo` | Prints adapter/device info to a log |
| `02-BlueScreen` | First D3D11 window + swap chain, blue clear |
| `03-Perspective` | BW + color triangles / rectangles, triangle strip vs list |
| `04-Orthographic` | Orthographic projection |
| `05-2DRotation` / `05-3DRotation` | 2D / 3D rotation of triangle, rectangle, pyramid, cube |
| `06-Texture` | Textured pyramid, cube, both, smiley, tweaked smiley, checkerboard |
| `07-Sphere` | Sphere mesh (Sphere.lib), `DrawIndexed` |
| `08-Light` | Diffuse → per-vertex → per-pixel → PV/PF toggle → 2 lights on pyramid → 3 lights on sphere → 24 spheres |
| `09-Tessellation` | Tessellation shaders |
| `10-Geometry` / `11-GeometryNew` | Geometry shaders (newer variant) |
| `12-Interleaved` | Interleaved position/color/texcoord/normal vertex buffer |

## OpenGL

### Windows (`opengl/windows`)

Toolchain: Win32 + GLEW (programmable) + GLSL + `vmath.h` matrix helpers. Each project has a `Build.bat` run from a **Developer Command Prompt for Visual Studio**.

`01-Windowing`: `01-Windowing` · `02-Messages` · `03-HelloWorld` · `04-Icon` · `05-FullScreen` · `06-LogFile` · `07-FinalWindowsStub`

`02-OpenGL/01-FixedFunctionPipeline`:

| Topic | Variants |
|-------|----------|
| `01-BlueScreen`, `02-Triangle`, `03-GLInfo` | window, triangle, driver info |
| `04-Ortho`, `05-Perspective` | BW + colored → triangle, rectangle, 2 shapes |
| `06-2DRotation`, `07-3DRotation` | BW + colored → triangle/rectangle, pyramid/cube/both |
| `08-LoadMultMatrix`, `09-LookAt` | matrix loading, camera setup |
| `10-DepthTemplate` | depth buffer (triangle, rectangle, no-shape) |
| `11-MatrixStack` | solar system, robotic arm (plus `HW` sub-variants) |
| `12-Texture` | checkerboard, one 3D shape (pyramid/cube), smiley, tweaked smiley, two shapes |
| `13-LightTemplate`, `14-Light` | templates + diffuse, Gouraud shading, 2 lights on spinning pyramid, 3 moving lights on steady sphere, spot light, 24-sphere materials |
| `15-SpecialEffect`, `16-UtahTeaPot` | special effect, GLUT teapot |

`02-OpenGL/02-ProgrammablePipeline`:

| Topic | Content |
|-------|---------|
| `01-BlueScreenWithGLEW`, `02-BlueScreenWithEmptyShaders` | GLEW setup, empty shader program |
| `03-OrthographicProjection`, `04-Perspective` | projection via GLSL + `vmath.h` |
| `05-2DRotation`, `06-3DRotation` | rotations |
| `07-Texture` | textured pyramid, cube, 2 shapes, smiley, tweaked smiley, checkerboard |
| `08-Sphere` | sphere mesh |
| `09-Light` | diffuse → per-vertex → per-fragment → PV/PF toggle → 2 lights on pyramid → 3 lights on sphere → 24 spheres |
| `10-FBO(RenderToTexture)` | render-to-texture |
| `11-TessellationShader`, `12-GeometryShader` | tessellation & geometry shaders |
| `13-Interleaved`, `14-IndexedDrawing` | interleaved VBOs, indexed draws |
| `15-ModelLoading` | `.obj` loading (Suzanne), sessions 1–5 |
| `16-HeterogeneousParallelProgramming` | GL sine wave, OpenGL-CUDA, OpenGL-OpenCL |
| `17-GraphPaper`, `18-SolarSystem`, `19-RoboticArm` | shader-based scene builds |

### Linux (`opengl/linux`)

Toolchain: X11/Xlib + GLX, `build.sh` with `gcc` (`-lX11 -lGL`, plus `-lGLEW` for the programmable pipeline).

`01-Windowing`: `01-Window` · `02-CenteredWindow` · `03-Messages` · `04-FullScreen` · `05-HelloWorld` · `06-LogFile`

`02-OpenGL/01-FixedFunctionPipeline` mirrors the Windows FFP set: `01-GLUT`, `02-BlueScreen`, `02-Triangle`, `03-GLInfo`, `04-Ortho`, `05-Perspective`, `06-2DRotation`, `07-3DRotation`, `08-LoadMultMatrix`, `09-LookAt`, `10-DepthTemplate`, `11-MatrixStack` (solar system + `HW`, robotic arm), `12-Texture`, `13-LightTemplate`, `14-Light` (diffuse, Gouraud, 2 lights on spinning pyramid, 3 moving lights on steady sphere, spot light, 24-sphere materials), `15-SpecialEffect`, `16-UtahTeaPot`.

`02-OpenGL/02-ProgrammablePipeline`: `01-BlueScreen` · `02-Ortho` · `04-Perspective` · `05-2DRotation` · `06-3DRotation` · `07-Texture` · `08-Sphere` · `09-Light` · `10-FBO` · `11-Tessellation` · `12-Geometry` · `13-Interleaved` · `14-IndexDrawing` · `15-HeterogeneousParallelProgramming` (GL sine wave, OpenGL-CUDA, OpenGL-OpenCL).

### macOS (`opengl/mac`)

Toolchain: Cocoa / `NSOpenGLView` + `CVDisplayLink`, compiled with `clang` into `.app` bundles against the (deprecated) Apple OpenGL framework. Uses the core profile (`<OpenGL/gl3.h>`), so **all** programs are shader-based (GLSL + `vmath.h`).

`01-Windowing`: `01-Window` · `02-MessageBox` · `03-LogFile` · `04-JustWindow` · `05-CustomView` · `06-EventHandling` · `07-FullScreen`

`02-OpenGL`: `01-BlueScreen` · `02-Ortho` · `03-Perspective` · `04-2DRotation` · `05-3DRotation` · `06-Texture` (pyramid, cube, both, smiley, tweaked smiley, checkerboard) · `07-Sphere` · `08-Light` (diffuse, per-vertex, per-fragment, PV/PF toggle, 2 lights on pyramid, 3 lights on sphere, 24 spheres) · `09-FBO` · `10-Tessellation` · `11-Geometry` · `12-Interleaved` · `13-IndexedDrawing`

### Android (`opengl/android`)

Toolchain: Java + a custom `GLESView` (`GLSurfaceView`), standard Gradle projects. Each app follows the same windowing → rendering flow.

Windowing apps: `01-Window` · `02-Template` · `03-SeparateTextView` · `04-FullScreen` · `05-Landscape` · `06-Events`

`OpenGL-ES` (GLSL ES shaders): `01-BlueScreen` · `02-PerspectiveTriangle` · `03-OrthographicProjection` · `04-Perspective` · `05-2DRotation` · `06-3DRotation` · `07-Texture` · `08-Sphere` · `09-Light` · `10-TessellationShader` · `11-GeometryShader` · `12-Interleaved` · `13-IndexedDrawing` · `14-FBO`

`RTR2023_SphereRelatedFiles_Android_04.05.2024` contains notes for porting the sphere mesh to Java/GLES.

### iOS (`opengl/ios`)

Toolchain: Objective-C with `OpenGLES.framework` (ES 3.0 via `EAGLContext`), built as Xcode projects (`Window2.xcodeproj`). Each project is a full iOS app (storyboard-less, ARC off).

`01-Windowing`: `Window`, `Window2`

`02-OpenGL`: `01-BlueScreen` · `02-OrthographicTriangle` · `03-Perspective` · `04-2DRotation` · `05-3DRotation` · `06-Texture` · `07-Sphere` · `08-Light` (diffuse, per-vertex, per-fragment, PV/PF toggle, 2 lights on pyramid, 3 lights on sphere, 24 spheres) · `09-FBO` · `10-Interleaved` · `11-IndexedDrawing`

### Web (`opengl/web`)

Toolchain: plain HTML/JS with **WebGL 2** (`canvas.getContext("webgl2")`), `requestAnimationFrame` loop, and matrix math in JS. Each project is a `Canvas.html` + `Canvas.js` pair.

`01-Windowing`: `01-Window` · `02-Events` · `03-FullScreen`

`02-WebGL`: `01-BlueScreen` · `02-Orthographic` · `03-Perspective` · `04-2DRotation` · `05-3DRotation` · `06-Texture` · `07-Sphere` · `09-Light` (diffuse, per-vertex, per-fragment, PV/PF toggle, 2 lights on pyramid, 3 lights on sphere, 24 spheres) · `10-Interleaved` · `11-IndexedDrawing` · `12-FBO`

## Lighting progression

The lighting folders grow uniformly across platforms:

| Step | Direct3D | OpenGL (programmable) | Fixed-function |
|------|----------|-----------------------|----------------|
| Diffuse | `08-Light/01-Diffuse` | `09-Light/01-Diffuse` | `14-Light/01-DiffuseLight` |
| Per-vertex | `08-Light/02-PerVertex` | `09-Light/02-PerVertexLight` | `14-Light/02-GouraudShading` |
| Per-fragment/pixel | `08-Light/03-PerPixel` | `09-Light/03-PerFragmentLight` | – |
| Vertex ↔ fragment toggle | `08-Light/04-PVPFToggle` | `09-Light/04-PVPFToggle` | – |
| Two lights on pyramid | `08-Light/05-2LightsOnPyramid` | `09-Light/05-2LightsOnPyramid` | `14-Light/03-TwoLightsOnSpinningPyramid` |
| Three lights on sphere | `08-Light/06-3LightsOnSphere` | `09-Light/06-3LightsOnSphere` | `14-Light/04-ThreeMovingLightsOnSteadySphere` |
| 24-sphere grid | `08-Light/07-24Spheres` | `09-Light/07-24Spheres` | `14-Light/06`/`07-24Spheres…` |

Plus `05-SpotLight` and material-based `24Spheres-Material` in the fixed-function pipeline.

## Building

### Windows — Direct3D and OpenGL

Each project contains a `Build.bat`:

```bat
cl.exe /c /EHsc OGL.c          rem or d3d.cpp
rc.exe OGL.rc
link.exe OGL.obj OGL.res user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
```

Run it from a **Developer Command Prompt for Visual Studio** so `cl`, `rc`, and `link` are on `PATH`. Programmable-pipeline projects additionally point at a local GLEW install (e.g. `/I C:\glew-2.1.0\include` and `/LIBPATH:C:\glew-2.1.0\lib\Release\x64`).

### Linux

Each project contains a `build.sh`:

```sh
gcc -c -o OGL.o OGL.c
gcc -o OGL OGL.o -lX11 -lGL
./OGL
```

Shader-based programs additionally link `-lGLEW`.

### macOS

Each project contains a `build.sh` that compiles with `clang` into an app bundle:

```sh
mkdir -p ogl.app/Contents/MacOS
clang -Wno-deprecated-declarations -o ogl.app/Contents/MacOS/ogl ogl.m \
      -framework Foundation -framework Cocoa -framework QuartzCore -framework OpenGL
```

### Android

Projects are standard Gradle apps under `opengl/android`. Build with the wrapper and install via `adb`:

```sh
cd opengl/android/OpenGL-ES/01-BlueScreen
./gradlew assembleDebug
adb -d install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat System.out:I *:S
```

### iOS

Open the project in Xcode and build/run:

```sh
open opengl/ios/02-OpenGL/01-BlueScreen/Window2.xcodeproj
```

### Web

`opengl/web` projects are plain HTML/JS. Open the `Canvas.html` file in any WebGL 2–capable browser.

## Notes

- Build artifacts and IDE caches are ignored via `.gitignore` — only source, shaders, textures, and prebuilt `.lib`/`.h` dependencies are tracked. The `assignments/` folder (C-language and bulk FFP assignments) is local-only and not pushed.
- Many projects include a `Screenshot…` or `Screen Recording…` file showing the expected output.
- Numbering is not strictly sequential across platforms — each platform's folder is self-contained.
- Prebuilt dependencies used by several projects: `Sphere.h`/`Sphere.lib` (sphere mesh), `DirectXTK.lib`/`WICTextureLoader.h` (D3D11 texture loading), and `xnamath` (XNA Math).
