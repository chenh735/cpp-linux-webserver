CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -g
TARGET := bin/server
SRC := main.cpp server/server.cpp connection/http_connection.cpp util/file.cpp util/socket.cpp http-1.1/request.cpp http-1.1/response.cpp

clean:
	rm -rf bin

build:
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: build
	./$(TARGET)

