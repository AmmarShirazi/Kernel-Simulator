#include "process.h"


Process::Process() {
    priority = 0;
    arrival_time = 0;
    cpu_time = 0;
    io_time = 0;
    time_t cpu_start_time = time(0);
    cpu_elapsed_time = 0;
}


bool operator<(const Process& p1, const Process& p2) {
 
    return p1.priority < p2.priority;
}
bool operator>(const Process& p1, const Process& p2) {
 
    return p1.priority > p2.priority;
}



bool Process::operator<(Process const& other) {
    return this->priority < other.priority;
}
bool Process::operator>(Process const& other) {
    return this->priority > other.priority;
}