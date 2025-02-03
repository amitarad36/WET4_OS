CC = g++
CFLAGS = -Wall -Wextra -Werror -std=c++11
TARGET = test.out
SRC = test.cpp malloc_3.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
