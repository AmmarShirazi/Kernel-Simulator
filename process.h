#ifndef PROCESS_H
#define PROCESS_H

#include <iostream>
#include <string>
#include <time.h>

struct Process {

    std::string process_name;
    int priority;
    double arrival_time;
    std::string process_type;
    double cpu_time;
    double io_time;
    
    clock_t cpu_start_time;
    double cpu_elapsed_time;
    friend bool operator<(const Process& p1, const Process& p2);
    
    friend bool operator>(const Process& p1, const Process& p2);
    bool operator<(Process const& other);
    bool operator>(Process const& other);
    Process();

};

struct ComparePriority {
    bool operator() (Process* p1, Process* p2) {
        return p1->priority < p2->priority; //calls your operator
    }
};

struct CompareRemainingTime {
    bool operator() (Process* p1, Process* p2) {
        return (p1->cpu_time - p1->cpu_elapsed_time) > (p2->cpu_time - p2->cpu_elapsed_time); //calls your operator
    }
};

#endif
