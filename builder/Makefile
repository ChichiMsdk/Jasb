STD			=-std=c23
CFLAGS		=-fno-omit-frame-pointer 
# CFLAGS		+=-fdebug-info-for-profiling
# CFLAGS		+=-flto
CFLAGS		+=-fsanitize=address
CFLAGS		+=-g3
DEFINES		=-DTRACY_CALLSTACK=62 -DTRACY_ENABLE
ICFLAGS		=-IC:/tracy/public -IC:/tracy/public/tracy -Ibuilder
LFLAGS		=-fsanitize=address
# LFLAGS		+=-flto
# LFLAGS		+=-fuse-ld=lld 
LIBS		=

NAME		=jasb
BUILD_DIR	=build
OBJ_DIR		=$(BUILD_DIR)/jasb
EXTENSION	=.exe
OUTPUT		=$(BUILD_DIR)/$(NAME)$(EXTENSION)

SRC			=builder/jasb.c builder/jasb_compile_commands.c builder/jasb_strings.c\
			 builder/jasb_utils.c builder/jasb_errors.c builder/jasb_execute.c

SRC			=builder/jasb.c

OBJS		=$(OBJ_DIR)/jasb.o $(OBJ_DIR)/jasb_compile_commands.o\
			 $(OBJ_DIR)/jasb_strings.o $(OBJ_DIR)/jasb_utils.o \
			 $(OBJ_DIR)/jasb_errors.o $(OBJ_DIR)/jasb_execute.o

OBJS		=$(OBJ_DIR)/jasb.o

TRACY_OBJ	=$(OBJ_DIR)/TracyClient.o 

.PHONY: clean re

all: clean $(OUTPUT)

$(OUTPUT): $(OBJS) $(TRACY_OBJ)
	clang $(LFLAGS) $(STD) -g3 $^ -o $@ $(LIBS)

# test: $(OBJ_DIR)/TracyClient.o
# 	clang $(STD) -g3 build/obj/jasb.o build/obj/TracyClient.o -o build/jasb.exe

$(OBJ_DIR)/%.o: src/%.cpp
	clang++ $(DEFINES) $(ICFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: builder/%.c
	clang  $(LFLAGS) $(CFLAGS) $(STD) -g3 $(DEFINES) $(ICFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/jasb*.o
	rm -f $(OBJ_DIR)/*.o

re: clean all
