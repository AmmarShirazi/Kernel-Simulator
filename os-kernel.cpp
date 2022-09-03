#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <csignal>
#include <cmath>

#include "os-kernel.h"


sig_atomic_t sigflag = 0;

void sighandler(int s)
{
  // std::cerr << "Caught signal " << s << ".\n"; // this is undefined behaviour
    sigflag = 1; // something like that
}


/*
    PLEASE FIX FILE MANAGEMENT OR YOU WILL REGRET IT LATER
    I BEG FUTURE ME, DO NOT REPEAT SAME MISTAKES
*/


int main(int argc, char **argv) { // argument count, argument list

    // os-kernal input_filename cpu_count type(r, f, p) [timeslice(round robin only)] output_filename.
    // upto 4 cpus = upto 4 threads, main thread for scheduling other threads.
    std::signal(SIGINT, sighandler);
    if (argc < 5) {
        std::cerr << "Not enough paramters" << std::endl;
        return -1;
    }



    std::string input_filename(argv[1]);
    int cpu_count = atoi(argv[2]);
    // std::cout << cpu_count << std::endl;
    // exit(0);
    std::string scheduling_type(argv[3]);
    float timeslice = -1;
    std::string output_filename = "NULL";



    if (argc == 6) {
        timeslice = atof(argv[4]);
        output_filename = argv[5];
    }
    else {
        output_filename = argv[4];
    }
    // reading parameters into variables/preprocessing
    std::vector<Process> process_list = read_file(input_filename);

    for (int i = 0; i < process_list.size(); i++) {

        // std::cout << process_list[i].process_name << "\t" << process_list[i].priority << "\t" << process_list[i].arrival_time << "\t" << process_list[i].process_type << "\t" << process_list[i].cpu_time << "\t" << process_list[i].io_time << "\t";
        // std::cout << std::endl;
    }

    Scheduler process_scheduler(process_list);
    // std::cout << ">>>> " << scheduling_type << std::endl;
    CPU execution_cpus(cpu_count);
    execution_cpus.set_scheduler(&process_scheduler);
    execution_cpus.set_cpu_type(scheduling_type, timeslice/10);
    process_scheduler.set_cpu(&execution_cpus);
    process_scheduler.set_sch(scheduling_type);
    process_scheduler.set_output_file(output_filename);
    process_scheduler.start_scheduler();

    pthread_t* cores = execution_cpus.get_thread_ids();

    while (true) {
        if (sigflag != 0) {
            std::cout << std::endl;
            for (int i = 0; i < cpu_count; i++)  {
                pthread_cancel(cores[i]);
            }
            pthread_cancel(process_scheduler.get_output_tid());
            
            exit(0);

        }
    }

    // for (int i = 0; i < cpu_count; i++)  {
    //     pthread_join(cores[i], nullptr);
    // }

    return 0;
}

bool compare_double(double A, double B)
{
   return std::abs(A - B) < 0.001;
}

double get_time_elapsed(timespec& start, timespec& finish) {

    double elapsed = 0;
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    return elapsed;
}


int get_random(int min, int max)
{
	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(generator);
}

int convert_seconds_to_mirco_seconds(double seconds) {

    return 1000000 * seconds;

}

void* cpu_thread(void* args) {

    CPU* cpu_obj = (CPU*)args;

    pthread_mutex_lock(&cpu_obj->prison);
    int cpu_id = cpu_obj->id_counter++;

    // std::cout << cpu_id << std::endl;
    pthread_mutex_unlock(&cpu_obj->prison);
    // std::cout << cpu_id << std::endl;
    for (;;) {


        // this is cpu in its on and idle state
        if (!cpu_obj->cpu_executing_process[cpu_id]) {
            // std::cerr << "CPU" << cpu_id << " IDLE\n";
            continue;
        }

        if (cpu_obj->cpu_type == "f") {
            timespec exec_start_time;
            clock_gettime(CLOCK_MONOTONIC, &exec_start_time);
            for (;;) {
                // this is execution of a process

                timespec curr_time;
                clock_gettime(CLOCK_MONOTONIC, &curr_time);
                cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time = get_time_elapsed(exec_start_time, curr_time);

                if (cpu_obj->cpu_executing_process[cpu_id]->io_time > 0) {
                    // std::cout << "here";
                    int io_random = get_random(0, 100000);
                    if (io_random < 1 || cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                        
                        pthread_mutex_lock(&cpu_obj->prison);
                        cpu_obj->sch_obj->yield(cpu_id, cpu_obj->cpu_executing_process[cpu_id]);
                        pthread_mutex_unlock(&cpu_obj->prison);
                        
                        break;
                    }
                }
                

                if (cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                    cpu_obj->sch_obj->terminate(cpu_obj->cpu_executing_process[cpu_id]);
                    cpu_obj->sch_obj->idle(cpu_id);
                    break;
                }

                // process is executing here
            }
        }
        else if (cpu_obj->cpu_type == "r") {
            

            timespec exec_start_time;
            clock_gettime(CLOCK_MONOTONIC, &exec_start_time);
            double local_time_elapsed = 0.0;
            for (;;) {
                // this is execution of a process
                
                timespec curr_time;
                clock_gettime(CLOCK_MONOTONIC, &curr_time);
                local_time_elapsed = get_time_elapsed(exec_start_time, curr_time);

                if (cpu_obj->cpu_executing_process[cpu_id]->io_time > 0) {
                    // std::cout << "here";
                    int io_random = get_random(0, 100000);
                    if (io_random < 1 || cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                        
                        pthread_mutex_lock(&cpu_obj->prison);
                        cpu_obj->sch_obj->yield(cpu_id, cpu_obj->cpu_executing_process[cpu_id]);
                        pthread_mutex_unlock(&cpu_obj->prison);
                        break;
                    }
                }


                if (cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time + local_time_elapsed >=  cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                    
                    cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time += local_time_elapsed;
                    cpu_obj->sch_obj->terminate(cpu_obj->cpu_executing_process[cpu_id]);
                    cpu_obj->sch_obj->idle(cpu_id);
                    break;
                }
                if (local_time_elapsed >= cpu_obj->timesplice) {

                    cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time += local_time_elapsed;
                    pthread_mutex_lock(&cpu_obj->prison);
                    cpu_obj->sch_obj->wake_up(cpu_obj->cpu_executing_process[cpu_id]);
                    pthread_mutex_unlock(&cpu_obj->prison);
                    cpu_obj->sch_obj->idle(cpu_id);
                    break;
                }

                // process is executing here
            }

        }
        else if (cpu_obj->cpu_type == "p") {

            timespec exec_start_time;
            clock_gettime(CLOCK_MONOTONIC, &exec_start_time);
            for (;;) {
                // this is execution of a process

                timespec curr_time;
                clock_gettime(CLOCK_MONOTONIC, &curr_time);
                cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time = get_time_elapsed(exec_start_time, curr_time);
                // std::cout << "here" << std::endl;
                if (cpu_obj->cpu_executing_process[cpu_id]->io_time > 0) {
                    
                    int io_random = get_random(0, 100000);
                    if (io_random < 1 || cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                        
                        pthread_mutex_lock(&cpu_obj->prison);
                        cpu_obj->sch_obj->yield(cpu_id, cpu_obj->cpu_executing_process[cpu_id]);
                        pthread_mutex_unlock(&cpu_obj->prison);
                        break;
                    }
                }
                
                pthread_mutex_lock(&cpu_obj->prison);
                
                if (cpu_obj->sch_obj->get_top_prioriry_ready_queue()) {

                    if (cpu_obj->cpu_executing_process[cpu_id]->priority < cpu_obj->sch_obj->get_top_prioriry_ready_queue()->priority) {
                        cpu_obj->sch_obj->push_priority_queue(cpu_obj->cpu_executing_process[cpu_id]); // check this
                        cpu_obj->sch_obj->idle(cpu_id);
                        pthread_mutex_unlock(&cpu_obj->prison);
                        break;
                    }
                    else {
                        pthread_mutex_unlock(&cpu_obj->prison);
                    }
                }
                else {
                    pthread_mutex_unlock(&cpu_obj->prison);
                }
                

                if (cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                    cpu_obj->sch_obj->terminate(cpu_obj->cpu_executing_process[cpu_id]);
                    cpu_obj->sch_obj->idle(cpu_id);
                    break;
                }
            }   // process is executing here

        }
        else if (cpu_obj->cpu_type == "s") {

            timespec exec_start_time;
            clock_gettime(CLOCK_MONOTONIC, &exec_start_time);
            for (;;) {
                // this is execution of a process

                timespec curr_time;
                clock_gettime(CLOCK_MONOTONIC, &curr_time);
                cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time = get_time_elapsed(exec_start_time, curr_time);
                // std::cout << "here" << std::endl;
                if (cpu_obj->cpu_executing_process[cpu_id]->io_time > 0) {
                    
                    int io_random = get_random(0, 100000);
                    if (io_random < 1 || cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                        
                        pthread_mutex_lock(&cpu_obj->prison);
                        cpu_obj->sch_obj->yield(cpu_id, cpu_obj->cpu_executing_process[cpu_id]);
                        pthread_mutex_unlock(&cpu_obj->prison);
                        break;

                    }
                }
                

                pthread_mutex_lock(&cpu_obj->prison);
                if (cpu_obj->sch_obj->get_top_remaining_ready_queue()) {
                    // std::cout << cpu_obj->cpu_executing_process[cpu_id]->cpu_time - cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time << std::endl;
                    if ((cpu_obj->cpu_executing_process[cpu_id]->cpu_time - cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time) > (cpu_obj->sch_obj->get_top_remaining_ready_queue()->cpu_time - cpu_obj->sch_obj->get_top_remaining_ready_queue()->cpu_elapsed_time)) {
                        cpu_obj->sch_obj->push_remaining_queue(cpu_obj->cpu_executing_process[cpu_id]);
                        cpu_obj->sch_obj->idle(cpu_id);
                        pthread_mutex_unlock(&cpu_obj->prison);
                        break;
                    }
                    else {
                        pthread_mutex_unlock(&cpu_obj->prison);
                    }
                }
                else {
                    pthread_mutex_unlock(&cpu_obj->prison);
                }
                

                if (cpu_obj->cpu_executing_process[cpu_id]->cpu_elapsed_time >= cpu_obj->cpu_executing_process[cpu_id]->cpu_time) {
                    cpu_obj->sch_obj->terminate(cpu_obj->cpu_executing_process[cpu_id]);
                    cpu_obj->sch_obj->idle(cpu_id);
                    break;
                }
            }   // process is executing here
            

        }

    }
    

    return nullptr;

}

std::vector<Process> read_file(std::string filename) { // FILE HANDLING FUNCTIONS ARE V UGLY

    std::fstream file(filename, std::ios::in | std::ios::out);
    if (file.fail()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {};
    }

    std::string dump_line;
    getline(file, dump_line);

    std::vector<Process> process_list;

    // use >>
    int i = 0;
    while (!file.eof()) {


        std::string buffer;
        getline(file, buffer);
        std::istringstream iss(buffer);
        std::string token;
        std::vector<std::string> temp;
        Process temp_p;

        if (buffer == "") {
            break;
        }

        while (getline(iss, token, '\t') ) {
            temp.push_back(token);
            // std::cout << token << std::endl;
        }

        if (temp.size() == 4) {                         // V UGLY AS EXPECTED
            temp_p.process_name = temp[0];
            temp_p.priority = std::stoi(temp[1]);
            temp_p.arrival_time = std::stoi(temp[2]);
            temp_p.process_type = temp[3];
            temp_p.cpu_time = get_random(0, 10); // randomize
            if (temp_p.process_type == "I") {
                // std::cout << "here";
                temp_p.io_time = get_random(1, 3); //randomize
            }
            else {
                temp_p.io_time = -1;
            }
        }
        else if (temp.size() == 6) {
            temp_p.process_name = temp[0];
            temp_p.priority = std::stoi(temp[1]);
            temp_p.arrival_time = std::stoi(temp[2]);
            temp_p.process_type = temp[3];
            temp_p.cpu_time = std::stoi(temp[4]);
            temp_p.io_time = std::stoi(temp[5]);
        }

        process_list.push_back(temp_p);

    }

    return process_list;

    // this is code...
    // i like dots...
    // this is if condition...
}



