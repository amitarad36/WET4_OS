# Compiler
CXX = g++

# Compilation Flags
CXXFLAGS = -Wall -Wextra -Werror -std=c++11

# Source Files
SRC = test.cpp malloc_2.cpp
OBJ = $(SRC:.cpp=.o)

# Output Executable
EXEC = test.out

# Default rule
all: $(EXEC)

# Compile the executable
$(EXEC): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXEC)

# Run the compiled test
run: $(EXEC)
	./$(EXEC)

# Clean generated files
clean:
	rm -f $(EXEC) $(OBJ)
