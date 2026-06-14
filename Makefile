CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17
TARGET := server
SRC := main.cpp

clean:
	rm -f $(TARGET)

build:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: build
	./$(TARGET)

