PROJ_NAME := shared

CC := gcc
CFLAGS := $(C_STD_FLAGS) $(C_DEBUG_FLAGS)
LDFLAGS := $(C_LD_DEBUG_FLAGS)

SRC_DIR := ./src
OBJ_DIR := $(if $(G_OBJ_DIR),$(G_OBJ_DIR)/$(PROJ_NAME),./obj)
LIB_DIR := $(OBJ_DIR)

SRC_FILES := $(shell find $(SRC_DIR) -name *.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(SRC_FILES))

DEP_OBJ_FILES := 

build_lib: $(LIB_DIR)/lib$(PROJ_NAME).a


# libshared.a
$(LIB_DIR)/lib$(PROJ_NAME).a: $(OBJ_FILES)
	ar rcs $@ $^ $(DEP_OBJ_FILES)


# *.c.o -> *.c
$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | obj_dir_check
	$(CC) $(CFLAGS) -c $^ -o $@


obj_dir_check:
	@mkdir -p $(dir $(OBJ_FILES))

.PHONY: clean

clean:
	rm -rf $(OBJ_DIR)
