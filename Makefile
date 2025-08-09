CC = gcc
CXX = g++

# Use system GLEW/GLFW on Linux via pkg-config
PKG_CFLAGS := $(shell pkg-config --cflags glew glfw3)
PKG_LIBS   := $(shell pkg-config --libs glew glfw3)

CFLAGS = -Wall -std=c99 -O2 $(PKG_CFLAGS) -DGLFW_INCLUDE_NONE -DGLEW_NO_GLU
CXXFLAGS = -Wall -O2 -DIMGUI_IMPL_OPENGL_LOADER_GLEW $(PKG_CFLAGS) -DGLFW_INCLUDE_NONE -DGLEW_NO_GLU
LDFLAGS = $(PKG_LIBS) -ldl -lm -lpthread -lstdc++

# Dear ImGui (expected at third_party/imgui)
IMGUI_DIR = third_party/imgui
IMGUI_SRC = \
	$(IMGUI_DIR)/imgui.cpp \
	$(IMGUI_DIR)/imgui_draw.cpp \
	$(IMGUI_DIR)/imgui_tables.cpp \
	$(IMGUI_DIR)/imgui_widgets.cpp \
	$(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
	$(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
IMGUI_OBJ = $(IMGUI_SRC:.cpp=.o)

# Name of the output executable
OUTPUT = app

# Source files and object files
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:.c=.o)
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJ = $(CPP_SRC:.cpp=.o)

all: $(OUTPUT)

$(OUTPUT): $(OBJ) $(CPP_OBJ) $(IMGUI_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -I $(IMGUI_DIR) -I $(IMGUI_DIR)/backends

debug: CFLAGS += -DDEBUG -O0 -g
debug: CXXFLAGS += -DDEBUG -O0 -g
debug: clean $(OUTPUT)

clean:
	rm -f $(OBJ) $(CPP_OBJ) $(IMGUI_OBJ) $(OUTPUT)
