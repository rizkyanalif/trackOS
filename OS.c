#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define MAX_PROCESSES 10

// Struktur untuk menyimpan informasi proses
typedef struct {
    int pid;
    char name[10];
    int burst_time;
    int remaining_time;
    int waiting_time;
    int turnaround_time;
    int completed;
} Process;

// Variabel global
Process processes[MAX_PROCESSES];
int num_processes = 0;
int slices = 0;
int current_process = -1;
int current_time = 0;
int slices_elapsed = 0;
int all_completed = 0;

// Deklarasi fungsi
void timer_handler(int signum);
void display_processes();
void run_scheduler();
void display_results();

int main(){
    printf("========================================\n");
    printf("Round Robin Time-Sharing Simulator\n");
    printf("Using Signals and Timers\n");
    printf("========================================\n\n");
    
    // Input jumlah proses
    printf("Enter number of processes (1-%d): ", MAX_PROCESSES);
    scanf("%d", &num_processes);
    
    if (num_processes < 1 || num_processes > MAX_PROCESSES) {
        printf("Invalid number of processes!\n");
        return 1;
    }
    
    // Input time slices
    printf("Enter time slices (in seconds): ");
    scanf("%d", &slices);
    
    if (slices < 1) {
        printf("Invalid time slices!\n");
        return 1;
    }
    
    // Input burst time untuk setiap proses
    printf("\n");
    for (int i = 0; i < num_processes; i++) {
        processes[i].pid = i + 1;
        sprintf(processes[i].name, "P%d", i + 1);
        
        printf("Enter burst time for Process %s: ", processes[i].name);
        scanf("%d", &processes[i].burst_time);
        
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].completed = 0;
    }
    
    // Tampilkan informasi proses
    display_processes();
    
    printf("Press Enter to start simulation...");
    getchar(); // Clear input buffer
    getchar(); // Wait for Enter
    
    // Jalankan scheduler
    run_scheduler();
    
    return 0;
}


void display_processes() {
    printf("\n========================================\n");
    printf("Process Queue:\n");
    printf("========================================\n");
    printf("%-8s %-12s %-12s\n", "Name", "Burst Time", "Status");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < num_processes; i++) {
        printf("%-8s %-12d %-12s\n", 
               processes[i].name, 
               processes[i].burst_time,
               "Ready");
    }
    printf("========================================\n\n");
}

// Handler untuk SIGALRM - dipanggil setiap tick
void timer_handler(int signum) {
    if (all_completed) {
        return;
    }

    current_time++;
    slices_elapsed++;

    // Update waiting time untuk proses yang menunggu
    for (int i = 0; i < num_processes; i++) {
        if (i != current_process && !processes[i].completed && processes[i].remaining_time > 0) {
            processes[i].waiting_time++;
        }
    }

    // Jika ada proses yang sedang berjalan
    if (current_process >= 0) {
        processes[current_process].remaining_time--;
        
        printf("[Time %d] Process %s is running (Remaining: %d)\n", 
               current_time, 
               processes[current_process].name, 
               processes[current_process].remaining_time);

        // Cek apakah proses selesai
        if (processes[current_process].remaining_time == 0) {
            processes[current_process].completed = 1;
            processes[current_process].turnaround_time = current_time;
            printf(">>> Process %s COMPLETED at time %d\n\n", 
                   processes[current_process].name, 
                   current_time);
            slices_elapsed = slices; // Force context switch
        }
    }

    // Context switch jika slices habis atau proses selesai
    if (slices_elapsed >= slices) {
        slices_elapsed = 0;
        
        // Cari proses berikutnya yang belum selesai
        int next_process = -1;
        int start_search = (current_process + 1) % num_processes;
        
        for (int i = 0; i < num_processes; i++) {
            int idx = (start_search + i) % num_processes;
            if (!processes[idx].completed && processes[idx].remaining_time > 0) {
                next_process = idx;
                break;
            }
        }

        if (next_process != -1) {
            if (current_process >= 0 && !processes[current_process].completed) {
                printf(">>> Context switch from %s to %s\n\n", 
                       processes[current_process].name, 
                       processes[next_process].name);
            }
            current_process = next_process;
        } else {
            // Semua proses selesai
            all_completed = 1;
            printf("\n========================================\n");
            printf("ALL PROCESSES COMPLETED!\n");
            printf("========================================\n\n");
            display_results();
        }
    }
}

void display_results() {
    printf("Final Results:\n");
    printf("========================================\n");
    printf("%-8s %-12s %-14s %-16s\n", 
           "Process", "Burst Time", "Waiting Time", "Turnaround Time");
    printf("----------------------------------------\n");
    
    float total_waiting = 0;
    float total_turnaround = 0;
    
    for (int i = 0; i < num_processes; i++) {
        printf("%-8s %-12d %-14d %-16d\n",
               processes[i].name,
               processes[i].burst_time,
               processes[i].waiting_time,
               processes[i].turnaround_time);
        
        total_waiting += processes[i].waiting_time;
        total_turnaround += processes[i].turnaround_time;
    }
    
    printf("========================================\n");
    printf("Average Waiting Time: %.2f\n", total_waiting / num_processes);
    printf("Average Turnaround Time: %.2f\n", total_turnaround / num_processes);
    printf("========================================\n");
}

void run_scheduler() {
    struct itimerval timer;
    
    // Setup signal handler untuk SIGALRM
    signal(SIGALRM, timer_handler);
    
    // Konfigurasi timer: 1 detik per tick
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    
    printf("\n========================================\n");
    printf("Starting Round Robin Scheduler\n");
    printf("Time slices: %d\n", slices);
    printf("========================================\n\n");
    
    // Mulai proses pertama
    current_process = 0;
    
    // Start timer
    setitimer(ITIMER_REAL, &timer, NULL);
    
    // Loop hingga semua proses selesai
    while (!all_completed) {
        pause(); // Wait untuk signal
    }
    
    // Stop timer
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
}
