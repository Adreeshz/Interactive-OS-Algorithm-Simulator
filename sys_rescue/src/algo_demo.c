#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "algo_demo.h"

void demo_bankers_algorithm(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║       BANKER'S ALGORITHM DEMONSTRATION             ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    BankersAlgorithm* banker = banker_init(3);
    
    printf("Initializing system with 3 resources:\n");
    printf("  Resource 0: 10 units\n");
    printf("  Resource 1: 5 units\n");
    printf("  Resource 2: 7 units\n\n");
    
    banker->available[0] = 10;
    banker->available[1] = 5;
    banker->available[2] = 7;
    banker->total[0] = 10;
    banker->total[1] = 5;
    banker->total[2] = 7;
    
    // Add processes
    for (int i = 0; i < 3; i++) {
        banker->processes[banker->process_count].pid = i + 1;
        banker->processes[banker->process_count].max_needed[0] = 3 + i;
        banker->processes[banker->process_count].max_needed[1] = 2 + i;
        banker->processes[banker->process_count].max_needed[2] = 2;
        banker->processes[banker->process_count].allocated[0] = 0;
        banker->processes[banker->process_count].allocated[1] = 0;
        banker->processes[banker->process_count].allocated[2] = 0;
        banker->process_count++;
    }
    
    printf("Processes added. Current state:\n");
    banker_display_state(banker);
    
    printf("\n\nAttempting resource allocation:\n");
    int result = banker_request_resource(banker, 1, 0, 3);
    printf("Process 1 requests 3 units of Resource 0: %s\n", result == 0 ? "✅ GRANTED" : "❌ DENIED");
    
    printf("\nFinal system state:\n");
    banker_display_state(banker);
    
    banker_destroy(banker);
    sleep(2);
}

void demo_scheduling_algorithms(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║       SCHEDULING ALGORITHMS DEMONSTRATION          ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("SCENARIO: 4 processes with varying burst times\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    // Create scheduler for FCFS
    printf("1️⃣  FIRST COME FIRST SERVED (FCFS)\n");
    Scheduler* fcfs = scheduler_init(SCHED_FCFS_ALG, 0);
    scheduler_add_process(fcfs, 1, 8, 0, 1);
    scheduler_add_process(fcfs, 2, 4, 1, 1);
    scheduler_add_process(fcfs, 3, 2, 2, 1);
    scheduler_add_process(fcfs, 4, 1, 3, 1);
    
    scheduler_fcfs(fcfs);
    scheduler_display_gantt_chart(fcfs);
    scheduler_destroy(fcfs);
    
    printf("\n\n2️⃣  ROUND ROBIN (Time Quantum = 4)\n");
    Scheduler* rr = scheduler_init(SCHED_RR_ALG, 4);
    scheduler_add_process(rr, 1, 8, 0, 1);
    scheduler_add_process(rr, 2, 4, 1, 1);
    scheduler_add_process(rr, 3, 2, 2, 1);
    scheduler_add_process(rr, 4, 1, 3, 1);
    
    scheduler_round_robin(rr);
    scheduler_display_gantt_chart(rr);
    scheduler_destroy(rr);
    
    printf("\n\n3️⃣  PRIORITY SCHEDULING\n");
    Scheduler* priority = scheduler_init(SCHED_PRIORITY_ALG, 0);
    scheduler_add_process(priority, 1, 8, 0, 1);
    scheduler_add_process(priority, 2, 4, 1, 3);
    scheduler_add_process(priority, 3, 2, 2, 2);
    scheduler_add_process(priority, 4, 1, 3, 4);
    
    scheduler_priority(priority);
    scheduler_display_gantt_chart(priority);
    scheduler_destroy(priority);
    
    sleep(2);
}

void demo_memory_management(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║     MEMORY MANAGEMENT - BUDDY SYSTEM               ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    MemoryManager* mem = memory_init(256, PLACE_BEST_FIT);
    
    printf("Initial state (256 bytes total):\n");
    memory_display_layout(mem);
    
    printf("\n\nAllocating 64 bytes for Process 1:\n");
    memory_allocate(mem, 1, 64);
    memory_display_layout(mem);
    
    printf("\n\nAllocating 48 bytes for Process 2:\n");
    memory_allocate(mem, 2, 48);
    memory_display_layout(mem);
    
    printf("\n\nAllocating 32 bytes for Process 3:\n");
    memory_allocate(mem, 3, 32);
    memory_display_layout(mem);
    
    printf("\n\nDeallocating Process 1:\n");
    memory_deallocate(mem, 1);
    memory_display_layout(mem);
    
    memory_destroy(mem);
    sleep(2);
}

void demo_page_replacement(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║      PAGE REPLACEMENT ALGORITHMS                   ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Memory has 4 frames. Reference string: 1,2,3,4,1,2,5,1,2,3,4,5\n\n");
    
    printf("LRU (Least Recently Used):\n");
    PageReplacementSystem* lru = page_system_init(4, PAGE_LRU);
    
    int pages[] = {1,2,3,4,1,2,5,1,2,3,4,5};
    for (int i = 0; i < 12; i++) {
        page_system_access(lru, pages[i]);
    }
    page_system_display_stats(lru);
    
    page_system_destroy(lru);
    
    printf("\n\nFIFO (First In First Out):\n");
    PageReplacementSystem* fifo = page_system_init(4, PAGE_FIFO);
    
    for (int i = 0; i < 12; i++) {
        page_system_access(fifo, pages[i]);
    }
    page_system_display_stats(fifo);
    
    page_system_destroy(fifo);
    
    sleep(2);
}

void demo_disk_scheduling(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║      DISK SCHEDULING ALGORITHMS                    ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Disk with 200 tracks. Request queue: 82, 170, 43, 140, 24, 16, 190\n");
    printf("Head position: 50\n\n");
    
    // FCFS
    printf("1️⃣  FCFS (First Come First Served):\n");
    DiskScheduler* fcfs = disk_scheduler_init(200, DISK_FCFS);
    fcfs->current_head_position = 50;
    
    int requests[] = {82, 170, 43, 140, 24, 16, 190};
    for (int i = 0; i < 7; i++) {
        disk_scheduler_add_request(fcfs, requests[i]);
    }
    
    disk_scheduler_fcfs(fcfs);
    disk_scheduler_display_sequence(fcfs);
    disk_scheduler_destroy(fcfs);
    
    // SSTF
    printf("\n\n2️⃣  SSTF (Shortest Seek Time First):\n");
    DiskScheduler* sstf = disk_scheduler_init(200, DISK_SSTF);
    sstf->current_head_position = 50;
    
    for (int i = 0; i < 7; i++) {
        disk_scheduler_add_request(sstf, requests[i]);
    }
    
    disk_scheduler_sstf(sstf);
    disk_scheduler_display_sequence(sstf);
    disk_scheduler_destroy(sstf);
    
    // C-SCAN
    printf("\n\n3️⃣  C-SCAN (Circular SCAN):\n");
    DiskScheduler* cscan = disk_scheduler_init(200, DISK_CSCAN);
    cscan->current_head_position = 50;
    
    for (int i = 0; i < 7; i++) {
        disk_scheduler_add_request(cscan, requests[i]);
    }
    
    disk_scheduler_cscan(cscan);
    disk_scheduler_display_sequence(cscan);
    disk_scheduler_destroy(cscan);
    
    sleep(2);
}
