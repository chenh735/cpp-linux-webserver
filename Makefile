CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -g
TARGET := server
SRC := main.cpp http-1.1/request.cpp http-1.1/request.hpp http-1.1/response.cpp http-1.1/response.hpp

clean:
	rm -f $(TARGET)

build:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: build
	./$(TARGET)

