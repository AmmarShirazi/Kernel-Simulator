#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <time.h>
#include <iomanip>
#include "scheduler.h"
#include <fstream>

void Scheduler::context_switch(int cpu_id, Process*  prcs) {
    context_swtich_count++;
    cpu_obj->get_execution_list()[cpu_id] = prcs;
}
void Scheduler::idle(int cpu_id) {

    cpu_obj->get_execution_list()[cpu_id] = nullptr;
}

void* schedule_waiting_queue(void* args) {

    Scheduler* sch_obj = (Scheduler*)args;
    // sch_obj->waiting_queue = sch_obj->cpu_obj->get_waiting_queue();

    for ( ; ; ) {
        
        pthread_mutex_lock(&sch_obj->cpu_obj->get_prison());
        int size = sch_obj->waiting_queue->size();
        pthread_mutex_unlock(&sch_obj->cpu_obj->get_prison());
        if (size != 0) {
            
            timespec curr_time_start;
            clock_gettime(CLOCK_MONOTONIC, &curr_time_start);
            double io_time = 2; // get_random(2, 2);
            timespec curr_time_end;
            clock_gettime(CLOCK_MONOTONIC, &curr_time_end);
            while (get_time_elapsed(curr_time_start, curr_time_end) < io_time) {
                clock_gettime(CLOCK_MONOTONIC, &curr_time_end);
            }
            
            pthread_mutex_lock(&sch_obj->cpu_obj->get_prison());
            Process* fin_process = sch_obj->waiting_queue->back();
            fin_process->io_time--;
            sch_obj->waiting_queue->pop_back();
            if (sch_obj->sch_type != "p" && sch_obj->sch_type != "s") {
                sch_obj->ready_queue->push_front(fin_process);
            }
            else if (sch_obj->sch_type == "p") {
                
                sch_obj->priority_ready_queue->push(fin_process);
            }
            else {
                sch_obj->remaining_time_priority_queue->push(fin_process);
            }

            pthread_mutex_unlock(&sch_obj->cpu_obj->get_prison());
        }
        
    }
    return nullptr;
}

void* schedule_new_processes(void* args) {

    Scheduler* sch_obj = (Scheduler*)args;
    for (int i = 0; i < sch_obj->process_list.size(); i++) {
        
        timespec curr_time;
        clock_gettime(CLOCK_MONOTONIC, &curr_time);

        while (get_time_elapsed(sch_obj->cpu_obj->get_start_time(), curr_time) < sch_obj->process_list[i].arrival_time) {

            clock_gettime(CLOCK_MONOTONIC, &curr_time);
        }
        
        pthread_mutex_lock(&sch_obj->scheduler_protection);
        
        
        if (sch_obj->sch_type != "p" && sch_obj->sch_type != "s") {
            sch_obj->ready_queue->push_front(&sch_obj->process_list[i]);
        }
        else if (sch_obj->sch_type == "p") {

            sch_obj->priority_ready_queue->push(&sch_obj->process_list[i]);
        }
        else {
            sch_obj->remaining_time_priority_queue->push(&sch_obj->process_list[i]);
        }
        
        pthread_mutex_unlock(&sch_obj->scheduler_protection);
    }
    
    return nullptr;
}

void* schedule_ready_queue(void* args) {

    Scheduler* sch_obj = (Scheduler*)args;
    if (sch_obj->sch_type != "p" && sch_obj->sch_type != "s") {
        for ( ; ; ) {
            pthread_mutex_lock(&sch_obj->scheduler_protection);
            
            if (sch_obj->ready_queue->size() > 0) {
                int free_cpu = sch_obj->cpu_obj->get_free_cpu_id();
                if (free_cpu != -1) {
                    pthread_mutex_lock(&sch_obj->cpu_obj->get_prison());
                    sch_obj->context_switch(free_cpu, sch_obj->ready_queue->back());
                    sch_obj->ready_queue->pop_back();
                    pthread_mutex_unlock(&sch_obj->cpu_obj->get_prison());
                }
            }
            pthread_mutex_unlock(&sch_obj->scheduler_protection);
            
        }
    }
    else if (sch_obj->sch_type == "p") {
        for ( ; ; ) {
            pthread_mutex_lock(&sch_obj->scheduler_protection);
            if (sch_obj->priority_ready_queue->size() > 0) {
                
                int free_cpu = sch_obj->cpu_obj->get_free_cpu_id();
                if (free_cpu != -1) {
                    pthread_mutex_lock(&sch_obj->cpu_obj->get_prison());
                    sch_obj->context_switch(free_cpu, sch_obj->priority_ready_queue->top());

                    sch_obj->priority_ready_queue->pop();
                    pthread_mutex_unlock(&sch_obj->cpu_obj->get_prison());
                }
            }
            pthread_mutex_unlock(&sch_obj->scheduler_protection);
        }
    }
    else {
        for ( ; ; ) {
            pthread_mutex_lock(&sch_obj->scheduler_protection);
            if (sch_obj->remaining_time_priority_queue->size() > 0) {
                int free_cpu = sch_obj->cpu_obj->get_free_cpu_id();
                
                if (free_cpu != -1) {
                    
                    pthread_mutex_lock(&sch_obj->cpu_obj->get_prison());
                    sch_obj->context_switch(free_cpu, sch_obj->remaining_time_priority_queue->top());

                    sch_obj->remaining_time_priority_queue->pop();
                    pthread_mutex_unlock(&sch_obj->cpu_obj->get_prison());
                }
            }
            pthread_mutex_unlock(&sch_obj->scheduler_protection);
        }
    }

    return nullptr;
}


void* log_thread(void* args) {
    Scheduler* sch_obj = (Scheduler*)args;
    sch_obj->log_size = 4 + sch_obj->cpu_obj->get_cpu_count();
    sch_obj->logs = new std::vector<std::string>[sch_obj->log_size];
    std::fstream file(sch_obj->output_filename, std::ios::in | std::ios::out | std::ios::trunc);
    if (file.fail()) {
        std::cerr << "Cant open file\n";
        exit(0);
    }
    timespec start = sch_obj->cpu_obj->start_of_time;
    double interval = 0.0;

    std::cout << std::left << std::setw(10) << "Time" << std::left << std::setw(10) << "Ru" << std::left << std::setw(10) << "Re" << std::left << std::setw(10) << "Wa";
    file << std::left << std::setw(10) << "Time" << std::left << std::setw(10) << "Ru" << std::left << std::setw(10) << "Re" << std::left << std::setw(10) << "Wa";
    for (int i = 0; i < sch_obj->cpu_obj->get_cpu_count(); i++) {
        std::cout << std::left << std::setw(20) << "CPU" + std::to_string(i);
        file << std::left << std::setw(20) << "CPU" + std::to_string(i);
        
    }

    double ready_state_time = 0.0;
    std::cout << "I/O Queue" << std::endl << std::endl;
    file << "I/O Queue" << std::endl << std::endl;

    for ( ; sch_obj->terminated_queue->size() < sch_obj->total_process_count; ) {
        
        
        std::cout << std::left << std::setw(10) << interval;
        file << std::left << std::setw(10) << interval;

        for (int i = 0; i < sch_obj->log_size; i++) {
            
            if (i < 3) {
                if (i == 0) {
                    std::cout << std::setw(10) << sch_obj->cpu_obj->get_running_count();
                    file << std::setw(10) << sch_obj->cpu_obj->get_running_count();
                }
                else if (i == 1) {
                    if (sch_obj->sch_type == "p") {
                        std::cout << std::left << std::setw(10) << sch_obj->priority_ready_queue->size();
                        ready_state_time += (sch_obj->priority_ready_queue->size() * 0.1);
                        file << std::left << std::setw(10) << sch_obj->priority_ready_queue->size();
                    }
                    else if (sch_obj->sch_type == "s") {
                        std::cout << std::left << std::setw(10) << sch_obj->remaining_time_priority_queue->size();
                        ready_state_time += (sch_obj->remaining_time_priority_queue->size() * 0.1);
                        file << std::left << std::setw(10) << sch_obj->remaining_time_priority_queue->size();
                    }
                    else {
                        std::cout << std::left << std::setw(10) << sch_obj->ready_queue->size();
                        ready_state_time += (sch_obj->ready_queue->size() * 0.1);
                        file << std::left << std::setw(10) << sch_obj->ready_queue->size();
                    }
                    
                }
                else {
                    std::cout << std::left << std::setw(10) << sch_obj->waiting_queue->size();
                    file << std::left << std::setw(10) << sch_obj->waiting_queue->size();
                }
            }
            else {
                if (i == sch_obj->log_size - 1) {
                    std::cout << "<<  ";
                    file << "<<  ";
                    for (int j = 0; j < sch_obj->waiting_queue->size(); j++) {
                        if (j == sch_obj->waiting_queue->size() - 1) {
                            std::cout << std::left << std::setw(20) << (*sch_obj->waiting_queue)[j]->process_name;
                            file << std::left << std::setw(20) << (*sch_obj->waiting_queue)[j]->process_name;
                        }
                        else {
                            std::cout << std::left << std::setw(20) << (*sch_obj->waiting_queue)[j]->process_name << "<<  ";
                            file << std::left << std::setw(20) << (*sch_obj->waiting_queue)[j]->process_name << "<<  ";
                        }
                        
                        
                    }
                    std::cout << "<<  ";
                }
                else {
                    if (sch_obj->cpu_obj->get_execution_list()[i - 3]) {
                        // sch_obj->logs[i].push_back(sch_obj->cpu_obj->get_execution_list()[i]->process_name);
                        std::cout << std::left << std::setw(20) << sch_obj->cpu_obj->get_execution_list()[i - 3]->process_name;
                        file << std::left << std::setw(20) << sch_obj->cpu_obj->get_execution_list()[i - 3]->process_name;
                    }
                    else {
                        std::cout << std::left << std::setw(20) << "IDLE";
                        file << std::left << std::setw(20) << "IDLE";
                        // sch_obj->logs[i].push_back("IDLE");
                    }
                }
                
            }
        }
        std::cout << std::endl;
        file << std::endl;
        interval += 0.1;
        usleep(100000);
        
    }
    std::cout << "Ready State Time: " << ready_state_time << std::endl;
    std::cout << "Context Switch Count: " << sch_obj->context_swtich_count << std::endl;
    std::cout << "Time Taken: " << interval << std::endl;
    file << "Ready State Time: " << ready_state_time << std::endl;
    file << "Context Switch Count: " << sch_obj->context_swtich_count << std::endl;
    file << "Time Taken: " << interval << std::endl;
    file.close();

    exit(0);
    return nullptr;
}