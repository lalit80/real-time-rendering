gcc -c -o OGL.o OGL.c
gcc -o OGL OGL.o -lX11 -lGL -lGLU -lSOIL
./OGL
rm OGL.o OGL log.txt
