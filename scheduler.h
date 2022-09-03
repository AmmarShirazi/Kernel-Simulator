#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <deque>
#include <pthread.h>
#include "process.h"
#include "os-kernel.h"
#include <queue>



void* schedule_new_processes(void* args);
void* schedule_ready_queue(void* args);
void* schedule_waiting_queue(void* args);
void* log_thread(void* args);
class CPU;

class Scheduler{

    std::priority_queue<Process*, std::vector<Process*>, ComparePriority>* priority_ready_queue;
    std::priority_queue<Process*, std::vector<Process*>, CompareRemainingTime>* remaining_time_priority_queue;
    std::deque<Process*>* waiting_queue;
    std::deque<Process*>* ready_queue;
    std::deque<Process*>* terminated_queue;
    std::vector<Process> process_list; // the ones read from file, this is our new procces state
    pthread_t* thread_ids;
    pthread_mutex_t scheduler_protection;
    CPU* cpu_obj;
    std::vector<std::string>* logs;
    pthread_t log_thread_id;
    std::string sch_type;
    std::string output_filename;
    int log_size;
    int context_swtich_count;
    int total_process_count;

public:

    Scheduler() {
        std::cerr << "No processed given to scheduler" << std::endl;
        exit(0);
    }
    Scheduler(std::vector<Process>& p_list) {
        pthread_mutex_init(&scheduler_protection, nullptr);
        thread_ids = new pthread_t[4];
        process_list = p_list;
        waiting_queue = new std::deque<Process*>;
        ready_queue = new std::deque<Process*>;
        terminated_queue = new std::deque<Process*>;
        priority_ready_queue = new std::priority_queue<Process*, std::vector<Process*>, ComparePriority>;
        remaining_time_priority_queue = new std::priority_queue<Process*, std::vector<Process*>, CompareRemainingTime>;
        context_swtich_count = 0;
        total_process_count = process_list.size();
    }

    friend void* schedule_new_processes(void* args);
    friend void* schedule_ready_queue(void* args);
    friend void* schedule_waiting_queue(void* args);
    friend void* log_thread(void* args);

    void set_sch(std::string sch) {
        sch_type = sch;
    }
    void set_cpu(CPU* cpu) {
        cpu_obj = cpu;
    }
    void set_output_file(std::string fname) {
        output_filename = fname;
    }
    void start_scheduler()  {
        
        
        pthread_create(&thread_ids[0], nullptr, schedule_new_processes, (void*)this);
        pthread_create(&thread_ids[1], nullptr, schedule_ready_queue, (void*)this);
        pthread_create(&thread_ids[2], nullptr, schedule_waiting_queue, (void*)this);
        pthread_create(&thread_ids[3], nullptr, log_thread, (void*)this);
    }

    pthread_t& get_output_tid() {
        return thread_ids[3];
    }

    Process* get_top_prioriry_ready_queue() {
        
        
        if (!priority_ready_queue->empty()) {
            return priority_ready_queue->top();
        }
        
        return nullptr;
    }

    Process* get_top_remaining_ready_queue() {
        
        
        if (!remaining_time_priority_queue->empty()) {
            return remaining_time_priority_queue->top();
        }
        
        return nullptr;
    }
    void push_remaining_queue(Process*& prcs) {
        remaining_time_priority_queue->push(prcs);
    }
    void push_priority_queue(Process*& prcs) {
        priority_ready_queue->push(prcs);
    }
    void push_front_waiting_queue(Process*& prcs) {
        waiting_queue->push_front(prcs);
    }
    void push_front_ready_queue(Process*& prcs) {
        ready_queue->push_front(prcs);
    }
    void terminate(Process*& prcs) {
        // easy xd add stuff to terminate queue bye bye
        terminated_queue->push_front(prcs);
    }

    void preempt() {
        
        
    }

    void wake_up(Process*& prcs) {
        ready_queue->push_front(prcs);
    }

    void yield(int cpu_id, Process*& prcs) {
        waiting_queue->push_front(prcs);
        idle(cpu_id);
    }

    void context_switch(int cpu_id, Process* prcs);
    void idle(int cpu_id);

};

void* schedule_new_processes(void* args);
void* schedule_ready_queue(void* args);
void* schedule_waiting_queue(void* args);

#endif