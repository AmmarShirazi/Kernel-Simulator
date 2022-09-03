#ifndef OS_H
#define OS_H

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <random>
#include <time.h>


#include "scheduler.h"
#include "process.h"
void* log_thread(void* args);
void* cpu_thread(void* args);
int get_random(int min, int max);
double get_time_elapsed(timespec& start, timespec& finish);
bool compare_double(double A, double B);
std::vector<Process> read_file(std::string filename); // this vector will be our process table and will contain its context information as well

class Scheduler;

class CPU {

    //bool* cpu_executing_state;// max 4 cpus allowed
    int cpu_count;
    pthread_t* thread_ids;
    pthread_mutex_t prison;
    pthread_mutex_t jail;
    Process** cpu_executing_process;
    Scheduler* sch_obj;
    timespec start_of_time;

    std::string cpu_type;
    double timesplice;
    
    int id_counter; // initialization variable for cpu ids.
public:

    

    CPU() {
        id_counter = 0;
        pthread_mutex_init(&prison, nullptr);
        pthread_mutex_init(&jail, nullptr);
        cpu_count = 1;
        // cpu_executing_state = new bool[1];
        cpu_executing_process = new Process*[1];
        thread_ids = new pthread_t[cpu_count];
        clock_gettime(CLOCK_MONOTONIC, &start_of_time);
        for (int i = 0; i < cpu_count; i++) {
            
            cpu_executing_process[i] = nullptr;
            pthread_create(&thread_ids[i], nullptr, cpu_thread, (void*)this);
        }
        
    }
    void set_scheduler(Scheduler* sch) {
        sch_obj = sch;
    }
    void set_cpu_type(std::string type, double timesplice) {
        if (type == "r") {
            cpu_type = type;
            this->timesplice = timesplice;
            return;
        }

        cpu_type = type;
        this->timesplice = -1;

    }
    
    

    CPU(int cpu_count) {
        id_counter = 0;
        pthread_mutex_init(&prison, nullptr);
        pthread_mutex_init(&jail, nullptr);
        this->cpu_count = cpu_count;
        // cpu_executing_state = new bool[cpu_count];
        cpu_executing_process = new Process*[cpu_count];
        thread_ids = new pthread_t[cpu_count];
        clock_gettime(CLOCK_MONOTONIC, &start_of_time);
        for (int i = 0; i < cpu_count; i++) {
            // cpu_executing_state[i] = false;
            cpu_executing_process[i] = nullptr;
            pthread_create(&thread_ids[i], nullptr, cpu_thread, (void*)this);
        }

    }

    
    pthread_mutex_t& get_prison() {
        return prison;
    }
    int get_running_count()  {

        int count = 0;

        for (int i = 0; i < cpu_count; i++) {
            if (cpu_executing_process[i]) {
                count++;
            }
        }

        return count;
    }
    int get_free_cpu_id() {
        for (int i = 0; i < cpu_count; i++) {
            if (!cpu_executing_process[i]) {
                return i;
            }
        }
        return -1;

    }
    timespec& get_start_time() {
        return start_of_time;
    }
    Process**& get_execution_list() {
        return cpu_executing_process;
    }
    bool* get_execution_state() {
        return nullptr;
        // return cpu_executing_state;
    }
    int get_cpu_count() {
        return cpu_count;
    }
    pthread_t* get_thread_ids() {
        return thread_ids;
    }
    friend void* log_thread(void* args);
    friend void* cpu_thread(void* args);
};

#endif


// add round dobin