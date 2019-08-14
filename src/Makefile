# Directories
INCDIR = ../include
OBJDIR = ../obj
SRCDIR = .

# Object files.
_OBJ = nerd.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

# Include files.
_INC = json.hpp
INC = $(patsubst %,$(INCDIR)/%,$(_INC))

# Compile options
CC = g++
CFLAGS = -Wall -std=c++11 -g -O0 -DSQLITE_DEBUG -I$(INCDIR)

# Link options.
LDFLAGS = -lboost_system -lsqlite3 -pthread


# target: dependencies
# 	actions
../nerd: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INC)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o
