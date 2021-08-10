PROJ_NAME := server

CC := gcc
CFLAGS := $(C_STD_FLAGS) $(C_DEBUG_FLAGS) -I$(G_PROJ_ROOT)/shared/inc/
LDFLAGS := $(C_LD_DEBUG_FLAGS) -L$(G_PROJ_ROOT)/obj/shared/ -lshared

SRC_DIR := ./src
OBJ_DIR := $(if $(G_OBJ_DIR),$(G_OBJ_DIR)/$(PROJ_NAME),./obj)
BIN_DIR := $(if $(G_BIN_DIR),$(G_BIN_DIR),./bin)

SRC_FILES := $(shell find $(SRC_DIR) -name *.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(SRC_FILES))

DEP_OBJ_FILES := 

build_bin: $(BIN_DIR)/$(PROJ_NAME)


# server -> build_obj
$(BIN_DIR)/$(PROJ_NAME): $(OBJ_FILES) | bin_dir_check
	$(CC) $^ $(DEP_OBJ_FILES) -o $@ $(LDFLAGS)

# *.c.o -> *.c
$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | obj_dir_check
	$(CC) $(CFLAGS) -c $^ -o $@


bin_dir_check:
	@mkdir -p $(BIN_DIR)

obj_dir_check:
	@mkdir -p $(dir $(OBJ_FILES))

.PHONY: clean

clean:
	rm -rf $(OBJ_DIR)
