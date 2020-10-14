LIBS= -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -pthread -lXi -ldl


all: main.cpp stb_image.cpp glad.c
	g++ -o main stb_image.cpp main.cpp glad.c -std=c++17 $(LIBS)

