# the name of the executable created
PROGRAM_NAME = snake

# Modify this variable as appropriate as you add .cpp files to
# your project
SRCS = cell.cpp grid.cpp segment.cpp snake.cpp pellet.cpp button.cpp buttonlist.cpp main.cpp

# you shouldn't need to modify below here, but you can
# if you know what you're doing

CXXFLAGS= -O3 -std=c++11
LDFLAGS= $(CXXFLAGS) $(LIBDIRS) -std=c++11
LDLIBS = -lfreeglut -lopengl32 -lglu32 -lglew32 -lm -lpng
OBJS=$(SRCS:%.cpp=%.o)
default: $(PROGRAM_NAME)
$(PROGRAM_NAME): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
clean:
	-@rm $(OBJS) $(PROGRAM_NAME).exe
.PHONY: default clean
