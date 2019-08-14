# Directories
INCDIR = include
BUILDDIR = build
SRCDIR = src

# Object files.
_OBJ = nerd.o
OBJ = $(patsubst %,$(BUILDDIR)/%,$(_OBJ))

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
$(BUILDDIR)/nerd: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(INC)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(BUILDDIR)/*.o
