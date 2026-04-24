# SYS_RESCUE: Interactive OS Algorithm Simulator

A terminal-based educational game that teaches core Operating System concepts through interactive gameplay.

## Overview

SYS_RESCUE is an escape room-style simulator where players act as the OS kernel to save a failing mainframe by mastering 7 major OS algorithms and concepts:

1. **Level 0**: Basic Linux Commands & Shell Scripting
2. **Level 1**: System Calls
3. **Level 2**: Synchronization Primitives (Mutexes, Semaphores)
4. **Level 3**: CPU Scheduling Algorithms (FCFS, RR, Priority)
5. **Level 4**: Banker's Algorithm & Deadlock Detection
6. **Level 5**: Memory Management & Disk Scheduling (Buddy System, FCFS, SSTF, SCAN, C-SCAN)
7. **Level 6**: Page Replacement & Virtual Memory (FIFO, LRU, Optimal)

## Features

- **7 Comprehensive Levels**: Progress through major OS concepts
- **Adaptive Difficulty**: Questions adjust based on your performance
- **Proficiency Tracking**: Track your progress on each level
- **Active Sessions Monitoring**: Admin panel shows 8 simulated concurrent players
- **Timed Challenges**: 30 minutes per level
- **Point System**: Up to 700 total points (100 per level)

## Quick Start

### Prerequisites
- GCC compiler
- POSIX-compliant system (Linux/Unix/macOS)
- Make build system

### Build & Run

```bash
cd sys_rescue
make clean && make
./sys_rescue.sh
```

### Default Credentials

**Regular User:**
- Username: `user1`
- Password: `pass123`

**Admin User:**
- Username: `admin`
- Password: `admin123`

## Gameplay

1. **Login/Register**: Create an account or use default credentials
2. **Select Level**: Start with Level 0 and progress sequentially
3. **Answer Questions**: 5 adaptive difficulty questions per level
4. **Track Progress**: View statistics and proficiency metrics
5. **Complete All Levels**: Achieve victory after completing all 7 levels

## Project Structure

```
sys_rescue/
├── src/                    # Source code
│   ├── main.c             # Game engine and levels
│   ├── algorithms.c       # OS algorithm implementations
│   ├── login_system.c     # User authentication
│   ├── question_pool.c    # Question management
│   ├── scheduler.c        # Scheduling module
│   ├── sync_engine.c      # Synchronization module
│   ├── game_infrastructure.c
│   ├── user_management.c
│   └── active_sessions.c  # Live player monitoring
├── include/               # Header files
├── data/                  # Game data (questions, user database)
├── assets/                # Game assets (logos, graphics)
├── obj/                  # Compiled object files
├── Makefile              # Build configuration
└── sys_rescue.sh         # Game launcher script
```

## Admin Features

Press `4` in the main menu to access the admin panel (admin login required):

- **View Active Sessions**: Monitor 8 simulated concurrent players
- **View Player Details**: See individual player statistics
- **Watch Live Updates**: Real-time player activity monitoring

## Scoring System

- **Per Question**: 20 points
- **Per Level**: 5 questions × 20 = 100 points
- **Total Game**: 7 levels × 100 = 700 points maximum

## Difficulty Levels

- 🔹 **Beginner**: Fundamental concepts
- 🟡 **Intermediate**: Applied knowledge
- 🔶 **Advanced**: Complex scenarios
- 🔴 **Proficient**: Expert level

Difficulty adjusts automatically:
- Score ≥80%: Advance to next difficulty
- Score <50%: Drop to previous difficulty
- **New**: Every 2 correct answers increases difficulty (improved progression)

## Question Pool

The game now features **175 expertly-crafted questions** across all 7 levels:

| Level | Topic | Questions | Difficulty Levels |
|-------|-------|-----------|------------------|
| 0 | Linux Commands | 71 | Beginner → Proficient |
| 1 | System Calls | 60 | Beginner → Proficient |
| 2 | Synchronization | 58 | Beginner → Proficient |
| 3 | CPU Scheduling | 55 | Beginner → Proficient |
| 4 | Banker's Algorithm | 27 | Beginner → Proficient |
| 5 | Memory Management | 27 | Beginner → Proficient |
| 6 | Virtual Memory | 27 | Beginner → Proficient |
| **TOTAL** | **All Levels** | **175** | **4 Levels Each** |

**Features**:
- No repeated questions within a single level
- Rich diversity across all difficulty levels
- Comprehensive coverage of each OS topic

## Compilation

```bash
make clean    # Remove old build artifacts
make          # Compile the project
make help     # Show build options
```

## Technical Details

### Technologies Used
- POSIX Threads (pthreads)
- Mutex synchronization
- Thread-safe data structures
- Adaptive question system
- Real-time player simulation

### Key Algorithms
- Basic Linux Commands & Shell Scripting
- System Calls (fork, exec, wait, etc.)
- Mutex/Semaphore Synchronization
- FCFS, Round-Robin, Priority Scheduling
- Banker's Algorithm (Deadlock Avoidance)
- Buddy System Memory Allocation
- FIFO/LRU/Optimal Page Replacement
- SCAN/C-SCAN Disk Scheduling

## Gameplay Tips

1. **Start Easy**: Begin with Beginner difficulty to learn concepts
2. **Answer Carefully**: Focus on understanding, not just speed
3. **Use Hints**: Each question has a helpful hint
4. **Track Progress**: Check statistics regularly
5. **View Active Sessions**: Check admin panel to see other players (admin only)
6. **Master Each Level**: Complete all 7 levels to win the game
7. **Difficulty Progression**: Every 2 correct answers unlocks harder questions

## System Requirements

- **OS**: Linux, macOS, or Unix-like system
- **Compiler**: GCC or compatible C compiler
- **Memory**: 50+ MB available RAM
- **Terminal**: 80+ column width recommended

## Building from Source

```bash
# Navigate to project directory
cd sys_rescue

# Clean previous builds
make clean

# Compile all modules
make

# Run the game
./sys_rescue.sh
```

## Gameplay Duration

- **Per Level**: 15-30 minutes (depending on difficulty)
- **Full Game**: 3-4 hours for complete playthrough
- **Admin Monitoring**: Additional 30 minutes to explore features

## Educational Value

This simulator teaches:
- Operating system fundamentals
- Basic Linux commands and scripting
- System calls and process management
- Concurrency control and synchronization
- CPU scheduling optimization
- Deadlock detection and avoidance
- Memory management strategies
- Virtual memory principles
- I/O optimization techniques

## Version Information

- **Version**: 2.0.0
- **Levels**: 7 complete levels
- **Questions**: 175 total adaptive difficulty questions (25+ per level)
- **Max Score**: 700 points (100 per level)
- **Status**: Production Ready ✅

## Recent Improvements (v2.0.0)

- ✅ **Enhanced Difficulty System**: Threshold reduced from every 3 to every 2 correct answers for better progression
- ✅ **Expanded Question Pool**: 175 questions (up from 85) providing greater variety per level
- ✅ **No Question Repetition**: Advanced tracking prevents same question appearing twice in a level
- ✅ **Cleaned Codebase**: Removed unused code and consolidated documentation
- ✅ **Zero Compilation Errors**: Fully tested and optimized

## Getting Help

1. **In-Game Help**: Press `3` in the main menu for tutorials
2. **View Statistics**: Press `2` to track your progress
3. **Admin Panel**: Press `4` to view system status (admin only)

---

**Ready to become an OS master?** Start the game and save the mainframe! 🚀
