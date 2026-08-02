gcc -c -o OGL.o OGL.c
gcc -o OGL OGL.o -lX11 -lGL -lGLU -lm
./OGL
rm OGL.o OGL log.txt
