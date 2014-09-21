# ---------------------------------------------------------------------------
CC	 := g++
LINK	:= g++
RM	 := rm -rf
MV	 := mv
TAR	 := tar
MKDIR	:= mkdir


# ---------------------------------------------------------------------------
#                   settings
# ---------------------------------------------------------------------------
SRC_SUFFIX	:= .cpp
OBJ_SUFFIX	:= .o
LIB_SUFFIX	:= .a
BIN_SUFFIX	:=
DLL_SUFFIX	:= .so

INC_PREFIX	:= -I
LIB_PREFIX	:= -L

OPT_C	 := -c
OPT_OUT	 := -o 
OPT_LINKOUT	:= -o

CFLAGS	 := $(OPT_C)
LIBFLAGS	:= -Debug 


# ---------------------------------------------------------------------------
#                   directories
# ---------------------------------------------------------------------------
SRC_DIR	 := ./src
OBJ_DIR	 := ./obj
INC_DIR	 := ./inc
LIB_DIR	 := ./lib /usr/local/lib /lib /usr/lib


# ---------------------------------------------------------------------------
#                   common settings
# ---------------------------------------------------------------------------
SRCS	 := $(wildcard $(SRC_DIR)/*$(SRC_SUFFIX))
OBJS 	 := $(patsubst $(SRC_DIR)/%$(SRC_SUFFIX),$(OBJ_DIR)/%$(OBJ_SUFFIX),$(SRCS)) 
INCS	 := $(addprefix $(INC_PREFIX), $(INC_DIR))
LIBS	 := $(addprefix $(LIB_PREFIX), $(LIB_DIR)) $(LIBFLAGS)
TEMPFILES 	:= core core.* *$(OBJ_SUFFIX) temp.* *.out typescript*

# ---------------------------------------------------------------------------
#                   make rule
# ---------------------------------------------------------------------------
TARGET	:= cmdtran

.PHONY:	 all clean

all:	$(TARGET)	
	@echo $(TARGET)	
	@echo $(SRCS)
	@echo $(OBJS)
	@echo $(INCS)
clean:
	$(RM) $(TARGET)$(BIN_SUFFIX) $(OBJS)

$(TARGET):$(OBJS)
	$(LINK) $(OPT_LINKOUT)$(TARGET)$(BIN_SUFFIX) $(LIBS) $(OBJS) -pthread

$(OBJS):$(OBJ_DIR)/%$(OBJ_SUFFIX):$(SRC_DIR)/%$(SRC_SUFFIX)
	$(CC) $(CFLAGS) $(INCS) $(OPT_OUT)$@ $<