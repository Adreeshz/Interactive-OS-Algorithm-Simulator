#!/bin/bash

##############################################################################
#                                                                            #
#  sys_rescue.sh - BOOTLOADER & LAUNCHER FOR INTERACTIVE OS SIMULATOR      #
#                                                                            #
#  The Narrative: The GUI is dead. Terminal is your only salvation.         #
#  The Tech: Bash script that checks permissions, compiles the engine,      #
#           and launches the compiled C binary with arguments.              #
#                                                                            #
#  Usage: ./sys_rescue.sh [--scheduler=TYPE] [--quantum=VALUE]            #
#         Example: ./sys_rescue.sh --scheduler=RR --quantum=4              #
#                                                                            #
##############################################################################

set -e  # Exit on error

# Color codes for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'  # No Color

# Configuration
BINARY="sys_rescue_engine"
MAKEFILE="Makefile"
LOGO_FILE="assets/boot_logo.txt"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

##############################################################################
# UTILITY FUNCTIONS
##############################################################################

print_header() {
    echo -e "${CYAN}╔════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   SYS_RESCUE: Bootloader Initialization Sequence   ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_status() {
    echo -e "${BLUE}[BOOTLOADER]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_ascii_logo() {
    if [ -f "$LOGO_FILE" ]; then
        echo ""
        cat "$LOGO_FILE"
        echo ""
    else
        print_warning "ASCII art file not found: $LOGO_FILE"
    fi
}

##############################################################################
# STEP 1: CLEAR SCREEN & DISPLAY BOOT SEQUENCE
##############################################################################

step_display_boot_screen() {
    clear
    print_header
    print_ascii_logo
    
    print_status "Initializing bootloader..."
    sleep 1
    print_status "Checking system prerequisites..."
    sleep 1
}

##############################################################################
# STEP 2: CHECK IF RUNNING FROM CORRECT DIRECTORY
##############################################################################

step_verify_directory() {
    print_status "Verifying project structure..."
    
    if [ ! -f "$SCRIPT_DIR/$MAKEFILE" ]; then
        print_error "Makefile not found in $SCRIPT_DIR"
        print_error "Please run this script from the sys_rescue project root directory."
        exit 1
    fi
    
    if [ ! -d "$SCRIPT_DIR/src" ] || [ ! -d "$SCRIPT_DIR/include" ]; then
        print_error "Project structure incomplete. Missing src/ or include/ directory."
        exit 1
    fi
    
    print_success "Project structure validated."
}

##############################################################################
# STEP 3: CHECK COMPILATION STATUS
##############################################################################

step_check_compilation() {
    print_status "Checking if engine binary exists..."
    
    if [ -f "$SCRIPT_DIR/$BINARY" ]; then
        if [ -x "$SCRIPT_DIR/$BINARY" ]; then
            print_success "Engine binary found and executable."
            RECOMPILE=false
        else
            print_warning "Engine binary found but NOT executable."
            chmod +x "$SCRIPT_DIR/$BINARY"
            print_success "Fixed permissions with chmod +x"
            RECOMPILE=false
        fi
    else
        print_warning "Engine binary not found. Compilation required."
        RECOMPILE=true
    fi
}

##############################################################################
# STEP 4: COMPILE THE ENGINE (IF NEEDED)
##############################################################################

step_compile_engine() {
    if [ "$RECOMPILE" = true ]; then
        print_status "Initiating compilation sequence..."
        print_status "Invoking Makefile: $MAKEFILE"
        
        cd "$SCRIPT_DIR"
        
        if make all; then
            print_success "Compilation completed successfully!"
            
            # Verify binary exists and is executable
            if [ -f "$BINARY" ] && [ -x "$BINARY" ]; then
                print_success "Engine binary ready for execution."
            else
                print_error "Compilation succeeded but binary is not executable."
                chmod +x "$BINARY" 2>/dev/null || {
                    print_error "Failed to set executable permission."
                    exit 1
                }
            fi
        else
            print_error "Compilation failed. Please check the Makefile and source files."
            exit 1
        fi
    fi
}

##############################################################################
# STEP 5: PARSE COMMAND-LINE ARGUMENTS
##############################################################################

step_parse_arguments() {
    print_status "Parsing launch arguments..."
    
    SCHEDULER_TYPE=""
    QUANTUM_VALUE=""
    
    for arg in "$@"; do
        case "$arg" in
            --scheduler=*)
                SCHEDULER_TYPE="${arg#*=}"
                print_status "Scheduler override: $SCHEDULER_TYPE"
                ;;
            --quantum=*)
                QUANTUM_VALUE="${arg#*=}"
                print_status "Time quantum: $QUANTUM_VALUE ms"
                ;;
            --help)
                print_usage
                exit 0
                ;;
            *)
                print_warning "Unknown argument: $arg"
                ;;
        esac
    done
}

##############################################################################
# STEP 6: BUILD EXECUTION COMMAND
##############################################################################

step_build_command() {
    print_status "Preparing engine launch sequence..."
    
    EXEC_CMD="$SCRIPT_DIR/$BINARY"
    
    if [ -n "$SCHEDULER_TYPE" ]; then
        EXEC_CMD="$EXEC_CMD --scheduler=$SCHEDULER_TYPE"
    fi
    
    if [ -n "$QUANTUM_VALUE" ]; then
        EXEC_CMD="$EXEC_CMD --quantum=$QUANTUM_VALUE"
    fi
    
    print_status "Execution command: $EXEC_CMD"
}

##############################################################################
# STEP 7: LAUNCH THE ENGINE
##############################################################################

step_launch_engine() {
    echo ""
    print_status "🚀 FINAL COUNTDOWN: 3... 2... 1..."
    sleep 1
    
    print_status "📡 Initiating kernel boot sequence..."
    print_status "🔴 Launching game engine..."
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    # Execute the binary with all arguments passed through
    if cd "$SCRIPT_DIR" && exec "$EXEC_CMD" "$@"; then
        # This line only executes if the binary crashes
        print_error "Engine terminated unexpectedly."
        exit 1
    else
        EXEC_STATUS=$?
        if [ $EXEC_STATUS -ne 0 ]; then
            print_error "Engine exited with status $EXEC_STATUS"
            exit $EXEC_STATUS
        fi
    fi
}

##############################################################################
# HELP MESSAGE
##############################################################################

print_usage() {
    cat << EOF
${BOLD}SYS_RESCUE: Interactive OS Algorithm Simulator${NC}

${BOLD}USAGE:${NC}
    $0 [OPTIONS]

${BOLD}OPTIONS:${NC}
    --scheduler=TYPE    Override scheduler algorithm (FCFS, SJF, PRIORITY, RR)
    --quantum=VALUE     Set time quantum for Round Robin (in milliseconds)
    --help             Display this help message

${BOLD}EXAMPLES:${NC}
    ./sys_rescue.sh                          # Launch with defaults
    ./sys_rescue.sh --scheduler=RR           # Launch with Round Robin
    ./sys_rescue.sh --scheduler=SJF          # Launch with Shortest Job First
    ./sys_rescue.sh --quantum=8              # Set time quantum to 8ms

${BOLD}GAME INFORMATION:${NC}
    An interactive, terminal-based survival game that teaches OS concepts
    through practical, hands-on puzzles about synchronization, scheduling,
    deadlocks, memory management, and disk I/O.

${BOLD}REQUIREMENTS:${NC}
    - GCC compiler with pthread support
    - Linux/Unix environment
    - Bash shell
    - Terminal with 60+ columns width

${BOLD}PROJECT STRUCTURE:${NC}
    sys_rescue/
    ├── sys_rescue.sh       # This bootloader script
    ├── Makefile            # Compilation instructions
    ├── src/
    │   ├── main.c          # Game loop & UI engine
    │   ├── sync_engine.c   # Synchronization primitives
    │   └── scheduler.c     # Task scheduling algorithms
    ├── include/
    │   ├── sync_engine.h   # Sync function prototypes
    │   └── scheduler.h     # Scheduler data structures
    └── assets/
        └── boot_logo.txt   # ASCII art bootscreen

EOF
}

##############################################################################
# MAIN EXECUTION SEQUENCE
##############################################################################

main() {
    step_display_boot_screen
    step_verify_directory
    step_check_compilation
    step_compile_engine
    step_parse_arguments "$@"
    step_build_command
    
    echo ""
    print_status "✅ All systems nominal. Bootloader complete."
    echo ""
    
    # Launch the engine (passes remaining arguments)
    step_launch_engine "$@"
}

# Start the bootloader if script is executed directly
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi
