rc.exe OGL.rc
nvcc.exe -I C:\glew-2.1.0\include -l C:\glew-2.1.0\lib\Release\x64 -o OGL.exe OGL.res user32.lib gdi32.lib OGL.cu
OGL
del *.exe  *.res *.obj *.txt
