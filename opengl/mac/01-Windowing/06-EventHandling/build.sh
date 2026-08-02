mkdir -p window.app/Contents/MacOS

clang -o window.app/Contents/MacOS/window window.m -framework Foundation -framework Cocoa
