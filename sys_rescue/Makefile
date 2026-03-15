# MAKEFILE for SYS_RESCUE: Interactive OS Algorithm Simulator
# Build the complete game engine with all synchronization and scheduling modules

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -O2 -g -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread -lm

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = .
ASSETS_DIR = assets

# Files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/sync_engine.c $(SRC_DIR)/scheduler.c $(SRC_DIR)/game_infrastructure.c
HEADERS = $(INC_DIR)/sync_engine.h $(INC_DIR)/scheduler.h $(INC_DIR)/game_infrastructure.h
OBJECTS = $(OBJ_DIR)/main.o $(OBJ_DIR)/sync_engine.o $(OBJ_DIR)/scheduler.o $(OBJ_DIR)/game_infrastructure.o
TARGET = $(BIN_DIR)/sys_rescue_engine

# Default target
all: directory $(TARGET)

# Create object directory
directory:
	@mkdir -p $(OBJ_DIR)
	@echo "📁 Created object directory: $(OBJ_DIR)"

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "🔗 Linking: $@"
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@chmod +x $@
	@echo "✅ Compilation successful!"
	@echo "🎮 Game engine ready: ./$(TARGET)"
	@echo ""
	@echo "📜 RUN: ./sys_rescue.sh to launch the game"

# Compile main.c
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(HEADERS)
	@echo "🔨 Compiling: $(SRC_DIR)/main.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile sync_engine.c
$(OBJ_DIR)/sync_engine.o: $(SRC_DIR)/sync_engine.c $(INC_DIR)/sync_engine.h
	@echo "🔨 Compiling: $(SRC_DIR)/sync_engine.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile scheduler.c
$(OBJ_DIR)/scheduler.o: $(SRC_DIR)/scheduler.c $(INC_DIR)/scheduler.h
	@echo "🔨 Compiling: $(SRC_DIR)/scheduler.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile game_infrastructure.c
$(OBJ_DIR)/game_infrastructure.o: $(SRC_DIR)/game_infrastructure.c $(INC_DIR)/game_infrastructure.h
	@echo "🔨 Compiling: $(SRC_DIR)/game_infrastructure.c (Synchronization Infrastructure)"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Clean object files and executable
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(TARGET)
	@echo "✅ Clean complete."

# Rebuild everything
rebuild: clean all

# Display build information
info:
	@echo "📋 SYS_RESCUE: Interactive OS Algorithm Simulator"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Linker Flags: $(LDFLAGS)"
	@echo ""
	@echo "Source files: $(SOURCES)"
	@echo "Header files: $(HEADERS)"
	@echo "Target binary: $(TARGET)"
	@echo ""
	@echo "Available targets:"
	@echo "  make all      - Build everything (default)"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make rebuild  - Clean and build"
	@echo "  make info     - Display this information"
	@echo "  make run      - Build and run the game"

# Run the game
run: $(TARGET)
	@echo "🎮 Starting SYS_RESCUE..."
	@./sys_rescue.sh

.PHONY: all directory clean rebuild info run
