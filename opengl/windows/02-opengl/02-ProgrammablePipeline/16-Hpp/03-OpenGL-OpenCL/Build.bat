cl.exe /c /EHsc /I C:\glew-2.1.0\include /I "C:\Program Files (x86)\OCL_SDK_Light\include" OGL.cpp
rc.exe OGL.rc
link.exe OGL.obj OGL.res /LIBPATH:C:\glew-2.1.0\lib\Release\x64 /LIBPATH:"C:\Program Files (x86)\OCL_SDK_Light\lib\x86_64" OpenCL.lib user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
OGL
del *.exe  *.res *.obj