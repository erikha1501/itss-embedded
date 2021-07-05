PROJ_NAME := client

CC := gcc
CFLAGS := $(C_STD_FLAGS) $(C_DEBUG_FLAGS)
LDFLAGS := $(C_LD_DEBUG_FLAGS)

SRC_DIR := ./src
OBJ_DIR := $(if $(G_OBJ_DIR),$(G_OBJ_DIR)/$(PROJ_NAME),./obj)
BIN_DIR := $(if $(G_BIN_DIR),$(G_BIN_DIR),./bin)

SRC_FILES := $(shell find $(SRC_DIR) -name *.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(SRC_FILES))

DEP_OBJ_FILES := 

build_bin: $(BIN_DIR)/$(PROJ_NAME)


# client -> build_obj
$(BIN_DIR)/$(PROJ_NAME): build_obj | bin_dir_check
	$(CC) $(OBJ_FILES) $(DEP_OBJ_FILES) -o $@ $(LDFLAGS)

build_obj: obj_dir_check $(OBJ_FILES)


# *.c.o -> *.c
$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@


bin_dir_check:
	@mkdir -p $(BIN_DIR)

obj_dir_check:
	@mkdir -p $(dir $(OBJ_FILES))

.PHONY: clean

clean:
	rm -rf $(OBJ_DIR)
