CC = g++
CFLAGS = -Wall -std=c++11 -g -O0
LDFLAGS = -lboost_system -lsqlite3 -pthread

DEPS = json.hpp
OBJ = nerd.o


# target: dependencies
# 	actions
nerd: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@
