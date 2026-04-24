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
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/sync_engine.c $(SRC_DIR)/scheduler.c $(SRC_DIR)/game_infrastructure.c \
          $(SRC_DIR)/user_management.c $(SRC_DIR)/question_pool.c $(SRC_DIR)/algorithms.c $(SRC_DIR)/login_system.c \
          $(SRC_DIR)/algo_demo.c $(SRC_DIR)/simulator.c $(SRC_DIR)/simulator_main.c $(SRC_DIR)/active_sessions.c
HEADERS = $(INC_DIR)/sync_engine.h $(INC_DIR)/scheduler.h $(INC_DIR)/game_infrastructure.h \
          $(INC_DIR)/user_management.h $(INC_DIR)/question_pool.h $(INC_DIR)/algorithms.h $(INC_DIR)/login_system.h \
          $(INC_DIR)/algo_demo.h $(INC_DIR)/simulator.h $(INC_DIR)/active_sessions.h
OBJECTS = $(OBJ_DIR)/main.o $(OBJ_DIR)/sync_engine.o $(OBJ_DIR)/scheduler.o $(OBJ_DIR)/game_infrastructure.o \
          $(OBJ_DIR)/user_management.o $(OBJ_DIR)/question_pool.o $(OBJ_DIR)/algorithms.o $(OBJ_DIR)/login_system.o \
          $(OBJ_DIR)/algo_demo.o $(OBJ_DIR)/simulator.o $(OBJ_DIR)/active_sessions.o
OBJECTS_SIMULATOR = $(OBJ_DIR)/simulator.o $(OBJ_DIR)/simulator_main.o $(OBJ_DIR)/user_management.o \
                    $(OBJ_DIR)/question_pool.o $(OBJ_DIR)/algorithms.o $(OBJ_DIR)/login_system.o \
                    $(OBJ_DIR)/sync_engine.o $(OBJ_DIR)/scheduler.o $(OBJ_DIR)/game_infrastructure.o $(OBJ_DIR)/algo_demo.o \
                    $(OBJ_DIR)/active_sessions.o
TARGET = $(BIN_DIR)/sys_rescue_engine
TARGET_SIMULATOR = $(BIN_DIR)/sys_rescue_simulator

# Default target
all: directory $(TARGET) $(TARGET_SIMULATOR)

# Create object directory
directory:
	@mkdir -p $(OBJ_DIR)
	@echo "📁 Created object directory: $(OBJ_DIR)"

# Link the executable
$(TARGET): $(OBJECTS)
	@echo "🔗 Linking: $@"
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@chmod +x $@
	@echo "✅ Game engine compiled: ./$(TARGET)"
	@echo ""

# Link the simulator executable
$(TARGET_SIMULATOR): $(OBJECTS_SIMULATOR)
	@echo "🔗 Linking: $@"
	@$(CC) $(OBJECTS_SIMULATOR) -o $@ $(LDFLAGS)
	@chmod +x $@
	@echo "✅ Simulator compiled: ./$(TARGET_SIMULATOR)"
	@echo ""
	@echo "🎮 GAME: ./sys_rescue.sh"
	@echo "🧪 SIMULATOR: ./sys_rescue_simulator [num_users]"
	@echo ""

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

# Compile user_management.c
$(OBJ_DIR)/user_management.o: $(SRC_DIR)/user_management.c $(INC_DIR)/user_management.h
	@echo "🔨 Compiling: $(SRC_DIR)/user_management.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile question_pool.c
$(OBJ_DIR)/question_pool.o: $(SRC_DIR)/question_pool.c $(INC_DIR)/question_pool.h
	@echo "🔨 Compiling: $(SRC_DIR)/question_pool.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile algorithms.c
$(OBJ_DIR)/algorithms.o: $(SRC_DIR)/algorithms.c $(INC_DIR)/algorithms.h
	@echo "🔨 Compiling: $(SRC_DIR)/algorithms.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile login_system.c
$(OBJ_DIR)/login_system.o: $(SRC_DIR)/login_system.c $(INC_DIR)/login_system.h
	@echo "🔨 Compiling: $(SRC_DIR)/login_system.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile algo_demo.c
$(OBJ_DIR)/algo_demo.o: $(SRC_DIR)/algo_demo.c $(INC_DIR)/algo_demo.h
	@echo "🔨 Compiling: $(SRC_DIR)/algo_demo.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile simulator.c
$(OBJ_DIR)/simulator.o: $(SRC_DIR)/simulator.c $(INC_DIR)/simulator.h
	@echo "🔨 Compiling: $(SRC_DIR)/simulator.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile simulator_main.c
$(OBJ_DIR)/simulator_main.o: $(SRC_DIR)/simulator_main.c $(INC_DIR)/simulator.h
	@echo "🔨 Compiling: $(SRC_DIR)/simulator_main.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Compile active_sessions.c
$(OBJ_DIR)/active_sessions.o: $(SRC_DIR)/active_sessions.c $(INC_DIR)/active_sessions.h
	@echo "🔨 Compiling: $(SRC_DIR)/active_sessions.c"
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Clean object files and executable
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(TARGET) $(TARGET_SIMULATOR)
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
	@echo "  make simulate - Build and run the multi-user simulator"

# Run the game
run: $(TARGET)
	@echo "🎮 Starting SYS_RESCUE..."
	@./sys_rescue.sh

# Run the simulator
simulate: $(TARGET_SIMULATOR)
	@echo "🧪 Starting Multi-User Simulator..."
	@./$(TARGET_SIMULATOR) 20

.PHONY: all directory clean rebuild info run simulate
