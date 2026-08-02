mkdir -p ogl.app/Contents/MacOS
mkdir -p ogl.app/Contents/Resources

clang++ -Wno-deprecated-declarations -o ogl.app/Contents/MacOS/ogl ogl.mm -framework Foundation -framework Cocoa -framework QuartzCore -framework OpenGL
