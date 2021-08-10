C_STD_FLAGS := -std=c17

G_BIN_DIR := $(shell realpath ./bin)
G_OBJ_DIR := $(shell realpath ./obj)
G_PROJ_ROOT := $(shell realpath .)

export C_STD_FLAGS
export G_BIN_DIR
export G_OBJ_DIR
export G_PROJ_ROOT

C_DEBUG_FLAGS := $(if $(DEBUG), -g -fsanitize=address -Wall -Wno-sign-compare,)
C_LD_DEBUG_FLAGS := $(if $(DEBUG), -fsanitize=address,)
export C_DEBUG_FLAGS
export C_LD_DEBUG_FLAGS

all: build_shared build_server build_client
	
build_shared:
	$(MAKE) -C shared

build_server:
	$(MAKE) -C server

build_client:
	$(MAKE) -C client


.PHONY: clean

clean:
	$(MAKE) -C shared clean
	$(MAKE) -C server clean
	$(MAKE) -C client clean

	rm -rf $(G_OBJ_DIR)
