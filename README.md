# Interactive OS Algorithm Simulator

## Project Overview

An interactive, terminal-based survival game built in C and Bash. Players assume the role of a system "Kernel" to save a catastrophically failing mainframe by solving real OS algorithmic puzzles. The game serves as a custom, lightweight engine that physically runs on the very OS concepts it teaches.

### Game Concept
Instead of reading textbooks, players engage in 5 high-stakes escape rooms, each targeting a specific OS challenge:
- **Level 0**: Terminal Boot Sequence
- **Level 1**: The Reactor Core (Synchronization)
- **Level 2**: The CPU Bottleneck (Scheduling)
- **Level 3**: The System Gridlock (Deadlock Avoidance)
- **Level 4**: The Memory Leak (Memory Allocation)
- **Level 5**: The Data Extraction (Disk Scheduling)

---

## Project Architecture

### File Structure
```
sys_rescue/
├── sys_rescue.sh              # Bootloader & launcher script (Bash)
├── Makefile                   # Build configuration
├── src/
│   ├── main.c                 # Game loop, UI, and level implementations
│   ├── sync_engine.c          # Producer-Consumer, Readers-Writers, Dining Philosophers
│   └── scheduler.c            # FCFS, SJF, Priority, Round Robin algorithms
├── include/
│   ├── sync_engine.h          # Synchronization function prototypes
│   └── scheduler.h            # Scheduler data structures
├── assets/
│   └── boot_logo.txt          # ASCII art boot screen
└── obj/                       # Compiled object files (auto-generated)
```

### Technology Stack
- **Languages**: C (core engine), Bash (wrapper/launcher)
- **Threading**: POSIX Threads (pthreads)
- **Synchronization**: Mutex, Semaphores, Read-Write Locks
- **Compilation**: GCC with pthread support
- **Environment**: Linux/Unix Terminal

---

## Build Instructions

### Prerequisites
- GCC compiler with pthread support
- GNU Make
- Linux/Unix environment
- Bash shell

### Compilation

#### Option 1: Using the Makefile directly
```bash
cd sys_rescue
make all          # Build everything
make clean        # Remove object files and binary
make rebuild      # Clean and rebuild
make info         # Display build information
```

#### Option 2: Using the bootloader script
```bash
cd sys_rescue
./sys_rescue.sh   # Automatically compiles if needed and launches game
```

#### Build Output
- **Binary**: `sys_rescue_engine` (compiled C executable)
- **Objects**: `obj/` directory (auto-created)
- **Size**: ~136 KB executable

---

## Running the Game

### Basic Launch
```bash
./sys_rescue.sh              # Launch with default settings
./sys_rescue.sh --help       # Display help message
```

### Advanced Options
```bash
./sys_rescue.sh --scheduler=RR --quantum=4      # Force Round Robin with 4ms quantum
./sys_rescue.sh --scheduler=SJF                 # Force Shortest Job First
```

### Command-Line Arguments
- `--scheduler=TYPE` - Override scheduler algorithm (FCFS, SJF, PRIORITY, RR)
- `--quantum=VALUE` - Set time quantum for Round Robin (milliseconds)
- `--help` - Display usage information

---

## Game Levels & Puzzles

### Level 0: Terminal Boot (Practicals 1 & 2)
**Concept**: Bash scripting and system permissions
**Puzzle**: Type the compile command to bootstrap the recovery engine
**Solution**: Type `make` or `make all`

### Level 1: Reactor Core (Practical 3)
**Concepts**: 
- Producer-Consumer synchronization
- Readers-Writers problem
- Dining Philosophers problem

**Available Demos**:
1. Producer-Consumer Event Logger (with semaphores)
2. Readers-Writers Game State (with R/W locks)
3. Dining Philosophers (Asymmetric solution)

### Level 2: CPU Bottleneck (Practical 4)
**Concepts**: Task scheduling algorithms
**Task Set**: 4 tasks with different burst times and priorities
**Algorithms**:
1. FCFS - First Come, First Serve (sequential)
2. SJF - Shortest Job First (optimal for this scenario)
3. Priority - Priority-based execution
4. Round Robin - Time-shared scheduling

**Optimal Solution**: SJF minimizes average waiting time

### Level 3: System Gridlock
**Concept**: Deadlock detection using Banker's Algorithm
**Puzzle**: Determine if system is in a safe state
**Answer**: YES (safe sequence exists: A → B → C)

### Level 4: Memory Leak
**Concept**: Memory allocation strategies
**Puzzle**: Choose optimal memory allocation strategy for fragmented RAM
**Options**:
1. First Fit
2. Best Fit ✓
3. Worst Fit
4. Buddy System ✓

### Level 5: Data Extraction (Final Level)
**Concept**: Disk scheduling (SSTF - Shortest Seek Time First)
**Puzzle**: Find optimal cylinder sequence to recover master password
**Fragments**: Located at cylinders 28, 86, 143, 190, 225
**Optimal Path**: 28 → 86 → 143 → 190 → 225

---

## Module Details

### sync_engine.h & sync_engine.c
Implements three classic synchronization problems:

#### Producer-Consumer Pattern
```c
typedef struct {
    char log_buffer[10][256];
    int head, tail, count;
    pthread_mutex_t log_mutex;
    sem_t empty;  // 10 slots initially
    sem_t full;   // 0 slots initially
} LogBuffer;
```
**Synchronization**: Semaphores + Mutex ensure thread-safe buffer access

#### Readers-Writers Problem
```c
typedef struct {
    int score;
    int time_left;
    int current_room;
    pthread_rwlock_t state_lock;  // Multiple readers, exclusive writers
} GameState;
```
**Synchronization**: Read-Write locks allow concurrent readers

#### Dining Philosophers
```c
pthread_mutex_t channels[5];  // 5 forks/channels
```
**Solution**: Asymmetric locking - {0,1,2,3} lock left→right; {4} locks right→left

### scheduler.h & scheduler.c
Implements four classic scheduling algorithms:

#### Data Structure
```c
typedef struct {
    int id;
    int burst_time;
    int priority;
    int arrival_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
} Task;
```

#### Algorithms
- **FCFS**: Execute in arrival order
- **SJF**: Sort by burst_time (ascending)
- **Priority**: Sort by priority value
- **Round Robin**: Time-share with quantum

#### Statistics Calculated
- Average waiting time
- Average turnaround time
- Total CPU time
- CPU utilization percentage

### main.c
Game engine with:
- **Game loop** and state management
- **5 levels** with interactive puzzles
- **Terminal UI** with status display
- **Score tracking** system
- **Boss queue** - processes user input and calculates solutions

---

## Bootloader Process (sys_rescue.sh)

### Execution Sequence
1. **Display Boot Screen**: ASCII art logo
2. **Verify Directory**: Check for Makefile and project structure
3. **Check Binary**: Look for pre-compiled executable
4. **Compile** (if needed): Run Makefile and verify permissions
5. **Parse Arguments**: Extract --scheduler and --quantum flags
6. **Launch Engine**: Execute binary with all arguments

### Error Handling
- Verifies project structure before compilation
- Automatically fixes missing execute permissions
- Clear error messages for troubleshooting
- Exit codes for scripting integration

---

## Compilation Details

### Compiler Flags
```
-Wall -Wextra         # Enable all common warnings
-std=c99              # C99 standard
-pthread              # POSIX threads support
-O2                   # Optimization level 2
-g                    # Debug symbols
-D_GNU_SOURCE         # GNU extensions
-D_POSIX_C_SOURCE=200809L  # POSIX features
```

### Linking
```
-pthread              # Link pthread library
-lm                   # Link math library
```

### Key Headers Used
- `<pthread.h>` - Threading primitives
- `<semaphore.h>` - Semaphores
- `<stdio.h>` - I/O operations
- `<stdlib.h>` - Memory management
- `<string.h>` - String operations

---

## Game Scoring

| Level | Task | Base Points |
|-------|------|-------------|
| 0 | Boot Sequence | 100 |
| 1 | Synchronization (any) | 150 |
| 1 | Full Analysis | 150 |
| 2 | Scheduler Puzzle | 200 |
| 2 | Full Analysis | 200 |
| 3 | Deadlock Detection | 250 |
| 4 | Memory Allocation | 250 |
| 5 | Disk Scheduling | 300 |

**Total Maximum Score**: 1,600 points

---

## Features Implemented

### ✅ Synchronization Engine (Practical 3)
- [x] Producer-Consumer with circular buffer and semaphores
- [x] Readers-Writers with read-write locks
- [x] Dining Philosophers with asymmetric solution
- [x] Thread creation and synchronization
- [x] Mutex locks and semaphore operations
- [x] File I/O in synchronized environment

### ✅ Scheduler Engine (Practical 4)
- [x] FCFS scheduling algorithm
- [x] SJF scheduling algorithm
- [x] Priority-based scheduling
- [x] Round Robin time-sharing
- [x] Performance statistics calculation
- [x] Turnaround and waiting time analysis
- [x] Array sorting for task ordering

### ✅ Interactive Game UI (Main Engine)
- [x] Game loop and state management
- [x] 5 playable levels with puzzles
- [x] Terminal-based user interface
- [x] Score tracking system
- [x] Status display
- [x] Input validation
- [x] ASCII art bootloader

### ✅ Boot System (Practicals 1 & 2)
- [x] Bash bootloader script
- [x] Automatic compilation checks
- [x] Permission verification
- [x] Argument parsing
- [x] Error handling
- [x] ASCII art display

---

## Troubleshooting

### Compilation Errors
**Issue**: `pthread_rwlock_t: unknown type`
- **Solution**: Ensure `-pthread` flag is in CFLAGS and LDFLAGS

**Issue**: `Makefile not found`
- **Solution**: Run script from sys_rescue directory

### Runtime Issues
**Issue**: Binary not found
- **Solution**: Run `make all` first, or use `./sys_rescue.sh`

**Issue**: Permission denied on sys_rescue_engine
- **Solution**: Run `chmod +x sys_rescue_engine` manually

### Game Issues
**Issue**: Logo file not found
- **Solution**: Run game from sys_rescue directory
- Game continues without logo if file missing

---

## Testing & Validation

### Compile Test
```bash
make clean && make all
```
Should produce: `✅ Compilation successful!`

### Binary Test
```bash
./sys_rescue_engine
```
Should display: Boot screen and main menu

### Full Integration Test
```bash
./sys_rescue.sh
```
Should run complete bootloader and game

---

## Educational Objectives

This simulator teaches:

1. **Synchronization Primitives**
   - Mutex locks and deadlock avoidance
   - Semaphores for resource counting  
   - Read-Write locks for concurrent access

2. **Process Scheduling**
   - Algorithm comparison and optimization
   - Performance metrics (waiting time, turnaround time)
   - Context switching overhead

3. **System Deadlocks**
   - Banker's Algorithm
   - Resource allocation safety
   - Cycle detection

4. **Memory Management**
   - Fragmentation and allocation strategies
   - Memory hierarchy implications

5. **Disk I/O Scheduling**
   - Seek time optimization
   - SSTF algorithm

6. **System Integration**
   - Bash scripting for system automation
   - Process compilation and execution
   - Command-line argument parsing

---

## Performance Characteristics

- **Binary Size**: ~136 KB
- **Compilation Time**: < 5 seconds
- **Startup Time**: < 1 second
- **Memory Usage**: ~5 MB (with pthread stacks)
- **Threading**: 5+ concurrent threads in synchronization levels

---

## Future Enhancement Ideas

1. Additional OS concepts (Virtual Memory, File Systems)
2. Network-based multiplayer mode
3. Persistent leaderboard (scores)
4. Extended difficulty levels
5. Custom scenario creation
6. Performance profiling tools
7. Breakpoint/single-step debugging mode
8. Documentation browser in-game

---

## License & Attribution

This project is an educational simulator created for OS coursework.

---

## Quick Reference

### File Purposes
| File | Purpose |
|------|---------|
| sys_rescue.sh | Boot and launch wrapper |
| Makefile | Build configuration |
| src/main.c | Game engine and UI |
| src/sync_engine.c | Synchronization implementations |
| src/scheduler.c | Scheduling algorithms |
| include/*.h | Function prototypes and data structures |
| assets/boot_logo.txt | ASCII art for bootscreen |

### Key Functions
- `game_loop()` - Main game state machine
- `level_*()` - Individual level implementations
- `pthread_*()` - POSIX thread operations
- `schedule_*()` - Scheduler algorithms
- `*_demo()` - Interactive demonstrations

---

**Ready to rescue the mainframe? Run ./sys_rescue.sh to begin!**
