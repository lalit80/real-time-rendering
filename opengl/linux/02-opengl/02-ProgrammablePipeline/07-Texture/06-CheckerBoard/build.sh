g++ -c -o OGL.o OGL.cpp
g++ -o OGL OGL.o -lX11 -lGL -lGLEW -lSOIL
./OGL
rm OGL.o OGL log.txt
