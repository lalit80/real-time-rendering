cl.exe /c /EHsc /I C:\glew-2.1.0\include OGL.c
rc.exe OGL.rc
link.exe OGL.obj OGL.res /LIBPATH:C:\glew-2.1.0\lib\Release\x64 user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
OGL
del *.exe  *.res *.obj