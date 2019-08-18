# Directories
EXTINCDIR = ../include
	# include files of external libs
OBJDIR = ../obj
SRCDIR = .
INCDIR = $(SRCDIR)
	# we store our own (internal)
	# headers in the source folder

# Object files.
_OBJ = http_server.o nerd.o sqlite_database.o sqlite_statement.o sqlite_table.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))


# Include files.
_EXTINC = json.hpp
EXTINC = $(patsubst %,$(EXTINCDIR)/%,$(_EXTINC))
_INC = http_server.h names.h sqlite_database.h sqlite_statement.h sqlite_table.h
INC = $(patsubst %,$(INCDIR)/%,$(_INC))

# Compile options
CC = g++
CFLAGS = -Wall -std=c++11 -g -O0 -DSQLITE_DEBUG -I$(EXTINCDIR)

# Link options.
LDFLAGS = -lboost_system -lsqlite3 -pthread


# target: dependencies
# 	actions
../nerd: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(EXTINC) $(INC)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o

print-%: ; @echo $* = $($*)
