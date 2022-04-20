CPPFLAGS = -D AMD64 -std=c++17

INC_PATH = -I"C:/opengl/include" 
LIBS_PATH = -L"C:/opengl/lib/glfw"
LIBS = -lglfw3 -lpthread

SRC_FILES = $(shell find . -name "*.cpp" -or -name "*.c")
OBJ_FILES = *.o

all:
	g++ -c $(CPPFLAGS) $(SRC_FILES) $(INC_PATH)
	g++ $(LIBS_PATH) $(OBJ_FILES) $(LIBS) -o Main.exe

clean:
	rm $(OBJ_FILES)
