mkdir -p ogl.app/Contents/MacOS

clang -Wno-deprecated-declarations -o ogl.app/Contents/MacOS/ogl ogl.m -framework Foundation -framework Cocoa -framework QuartzCore -framework OpenGL