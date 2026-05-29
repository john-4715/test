
CXX = g++
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -pthread 

TARGET = timer_app

SRCS = main.cpp timer.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) -pthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
