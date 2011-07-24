    /*

    Copyright (C) 2008  Dhruv Kumar dhruv21@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
    */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <stdarg.h>

#define OPCONTROL_PATH "/usr/local/bin/opcontrol"
#define OPREPORT_PATH "/usr/local/bin/opreport"
#define BATTERY_CAPACITY_FILE_PATH "/proc/acpi/battery/BAT0/state"
#define CPU_FREQUENCY_FILE_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define CPU_TIME_FILE_PATH "/proc/stat"
#define CPU0_TEMPERATURE_FILE_PATH "/proc/acpi/thermal_zone/THM0/temperature"
#define CPU1_TEMPERATURE_FILE_PATH "/proc/acpi/thermal_zone/THM1/temperature"
#define LOWTHRESHOLD 0.50
#define HIGHTHRESHOLD 0.70



const char CURRENT_SPEED_FILE[] = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed";

using namespace std;

extern char **environ;

long cycles_index_start = 0, cycles_field_width = 9, inst_index_start = 18, inst_field_width = 9;
long tab_cycles_index_start = 1, tab_cycles_field_width = 9, tab_inst_index_start = 19, tab_inst_field_width = 9;
long frequency_index = 0, frequency_field_width = 7;
double total_cycles=0, total_inst=0;
double FULL_CHARGE = 24480; //full capacity in mWh
long battery_capacity_index = 25, battery_capacity_field_width =6, battery_rate_index= 25 , battery_rate_field_width=6 ;
long battery_voltage_index = 25, battery_voltage_field_width = 6;
long temperature_index = 25, temperature_field_width = 2;
double diff_cycles = 0.0, diff_inst=0.0, previous_cycles=0.0, previous_inst =0.0, previous_rate=0.0, avg_rate=0.0;
double previous_voltage=0.0, avg_voltage = 0.0;
double previous_busy_time = 0.0, previous_total_time = 0.0;


int freqindex;
unsigned FREQUENCYARRAY[]={1000000, 1333000, 1667000, 2000000};

int old_capacity = 0, change_capacity=0;
long prev_cal_time;
int first_line_printed = 0;

// display an error message and exit the program
void die(bool system_error, const char *fmt, ...)
{
    fprintf(stderr, "Error: ");

    va_list ap;
    // get variable argument list passed
    va_start(ap, fmt);
    // display message passed on stderr
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");
    if (system_error)
        fprintf(stderr, "Error: %s\n", strerror(errno));

    exit(1);
}
void write_line(const char * filename, const char *fmt, ...)
{
    FILE * fp = fopen(filename, "w+");
    if (!fp)
        die(true, "Could not open file for writing: %s", filename);
    va_list ap;
    // get variable argument list passed
    va_start(ap, fmt);
    if (vfprintf(fp, fmt, ap) < 0)
        die(true, "Could not write to file: %s", filename);
    va_end(ap);
    fclose(fp);
}

int cpu_utilization(void) //second print sequence
{
	string line_stat_file;	
	ifstream proc_stat_file;
	proc_stat_file.open(CPU_TIME_FILE_PATH, ios::in);
	int linecount =0;
	unsigned long total_time, user_time, nice_time, system_time, idle_time, wait_time = 0;
	double busy_time;
	if(proc_stat_file.is_open())
	{
		getline(proc_stat_file, line_stat_file);
		char what[32];    		
    		sscanf(line_stat_file.c_str(), "%s %lu %lu %lu %lu %lu", what, &user_time, &nice_time, &system_time, &idle_time, &wait_time);
    		// count nice time as idle time
    		idle_time += nice_time;

    		// count IO wait time as idle time
    		idle_time += wait_time;

    		total_time = user_time + system_time + idle_time;
    		busy_time = (double)(user_time+ system_time);
    		//double cpu_utilization = (double)(busy_time/total_time);
    		
	}
	double interval_busy_time = busy_time - previous_busy_time;
	double interval_total_time = total_time - previous_total_time;
	double cpu_utilization = interval_busy_time/interval_total_time;
	cout << interval_busy_time << " " << interval_total_time << " " << cpu_utilization <<" ";
	previous_busy_time = busy_time;
	previous_total_time = total_time;
	proc_stat_file.close();
	
	ifstream current_frequency_file;
	current_frequency_file.open(CPU_FREQUENCY_FILE_PATH, ios::in);
	string line_frequency;
	if(current_frequency_file.is_open())
	{
		while(!current_frequency_file.eof())
		{
			getline(current_frequency_file, line_frequency);
			if(!(line_frequency == "\0"))
			{
				string present_frequency = line_frequency.substr(frequency_index, frequency_field_width);
				int int_frequency = atoi(present_frequency.c_str());
				cout << int_frequency << " ";
				if (int_frequency == 2000000) freqindex =3;
				else if (int_frequency == 1667000) freqindex = 2;
				else if (int_frequency == 1333000) freqindex = 1;
				else if (int_frequency == 1000000) freqindex = 0;
				//cout << freqindex <<" ";
			}
		}
	}
	
 
	
	write_line(CURRENT_SPEED_FILE, "%u\n", FREQUENCYARRAY[0]);
	
	//Temperatures
	
	ifstream cpu0_temperature_file;
	cpu0_temperature_file.open(CPU0_TEMPERATURE_FILE_PATH, ios::in);
	string line_cpu0_temp;
	if(cpu0_temperature_file.is_open())
	{
		while(!cpu0_temperature_file.eof())
		{
			getline(cpu0_temperature_file, line_cpu0_temp);
			if(!(line_cpu0_temp == "\0"))
			{
				string present_cpu0_temp = line_cpu0_temp.substr(temperature_index, temperature_field_width);
				int int_temp_cpu0 = atoi(present_cpu0_temp.c_str());
				cout << int_temp_cpu0 << " ";
			}	
			
		}
	}
	
	ifstream cpu1_temperature_file;
	cpu1_temperature_file.open(CPU1_TEMPERATURE_FILE_PATH, ios::in);
	string line_cpu1_temp;
	if(cpu1_temperature_file.is_open())
	{
		while(!cpu1_temperature_file.eof())
		{
			getline(cpu1_temperature_file, line_cpu1_temp);
			if(!(line_cpu1_temp == "\0"))
			{
				string present_cpu1_temp = line_cpu1_temp.substr(temperature_index, temperature_field_width);
				int int_temp_cpu1 = atoi(present_cpu1_temp.c_str());
				cout << int_temp_cpu1 << " ";
			}
				
			
		}
	}
	long currtime, time_difference;
	time_t temp;
	
	
	if(first_line_printed == 0)
		{
			
			cout<<"0"<<" ";
			prev_cal_time = time(&temp);
			first_line_printed = 1;
		}
	else
		{
			currtime = time(&temp);
			time_difference = currtime - prev_cal_time ;
			cout << time_difference<<" ";			
			
		}
	
	cout<<endl;	
	return 0;	
}

int cpi_calc() //first print sequence
{			
	long int_capacity, int_rate, int_voltage;
	string fileName="report";	
	string line;	
	ifstream inFile(fileName.c_str());	
	string cycles , inst ;	
	while (inFile)
	{
		getline(inFile, line);
		
		if ((line.find("\t") == string::npos) && !(line == "\0") )
		{			
			long int_cycles, int_inst;
			if(strlen(line.c_str()) >= (inst_index_start + inst_field_width))
				{
					cycles = line.substr(cycles_index_start, cycles_field_width);
					inst = line.substr(inst_index_start, inst_field_width);
					if(atoi(cycles.c_str())) 
					{
						if(atoi(inst.c_str()))
						{
							int_cycles = (long)atoi(cycles.c_str());
							int_inst = (long)atoi(inst.c_str());
							total_cycles += (double)int_cycles;
							total_inst += (double)int_inst;
						}
					}
				}	
		}
		else if (!(line == "\0"))
		{
			long int_cycles, int_inst;
			if(strlen(line.c_str()) >= (tab_inst_index_start + tab_inst_field_width))
				{
					cycles = line.substr(tab_cycles_index_start, tab_cycles_field_width);
					inst = line.substr(tab_inst_index_start, tab_inst_field_width);
					if(atoi(cycles.c_str()))
					{
						if(atoi(inst.c_str()))
						{
							int_cycles = (long)atoi(cycles.c_str());
							int_inst = (long)atoi(inst.c_str());
							total_cycles += (double)int_cycles;
							total_inst += (double)int_inst;
						}
					}
				}			
		}
	}
	
	inFile.close();	
	
	diff_cycles = fabs((total_cycles - previous_cycles));
	diff_inst = fabs((total_inst - previous_inst));
	double cpi = diff_cycles/diff_inst;
	cout << cpi << " ";
	cout << total_cycles << " " << total_inst << " " << diff_cycles << " " << diff_inst << " ";	
	previous_cycles = total_cycles;
	previous_inst = total_inst;	
	
	string line_bat_state;	
	ifstream battery_capacity_file;
	battery_capacity_file.open(BATTERY_CAPACITY_FILE_PATH, ios::in);
	int linecount =0;
	if(battery_capacity_file.is_open())
	{
		while(!battery_capacity_file.eof())
		{
			getline(battery_capacity_file, line_bat_state);
			++linecount;
			if (linecount == 4)
			{
				string present_rate = line_bat_state.substr(battery_rate_index, battery_rate_field_width);
				int_rate = atoi(present_rate.c_str());
				cout << int_rate << " " ;
				avg_rate = (previous_rate + int_rate)/2.0;
				cout << avg_rate << " ";
				previous_rate = int_rate;
			}
			if (linecount == 5)
			{
				string present_capacity = line_bat_state.substr(battery_capacity_index, battery_capacity_field_width);
				int_capacity = atoi(present_capacity.c_str());
				
				cout << int_capacity <<" " ;   
				change_capacity = old_capacity-int_capacity;   
				
				cout << change_capacity << " " << change_capacity/FULL_CHARGE <<" ";   
				old_capacity = int_capacity;   
			}		
			
			if (linecount == 6)
			{
				string present_voltage = line_bat_state.substr(battery_voltage_index, battery_voltage_field_width);
				int_voltage = atoi(present_voltage.c_str());
				cout << int_voltage << " " ;
				avg_voltage = (previous_voltage + int_voltage)/2.0;
				cout << avg_voltage << " ";
				previous_voltage = int_voltage;
				cout << (double)avg_rate/avg_voltage << " "; //average current drawn in Amperes I=P/V
				cout << (double)(cpi*avg_rate)/avg_voltage<<" ";/////avgCURRENTxCPI
			}
		}
	}
	
	
	battery_capacity_file.close(); 
		
	double cpi_x_bd = cpi*change_capacity/FULL_CHARGE;
	cout << cpi_x_bd << " ";
	double cpi_x_avgrate = cpi* avg_rate;
	cout << cpi_x_avgrate << " "; 
	
	return 0;		
}



static void exec_opcontrol(void)
{
	static char * argv[] = {"opcontrol", "--start", ">>opcontrolmsg", NULL}; 	
	execve(OPCONTROL_PATH, argv, environ);	
	fprintf(stderr, "%s: execve(2)\n", strerror(errno));
}

static void exec_opreport(void)
{	
	static char *argv[] = {"opreport", "--threshold", "4.0", "--no-header", NULL};	
	execve(OPREPORT_PATH, argv, environ);
	//close(fd);	
	fprintf(stderr, "%s: execve(2)\n", strerror(errno));
}

int opreport_generate(void)
{	
	time_t opreport_start_time, opreport_end_time;	
	pid_t pid_child_opreport;
	pid_t wait_pid_child_opreport;
	int status_exit_wait_opreport;
			
	pid_child_opreport = fork();
	
	if(pid_child_opreport == -1)
	{
		fprintf(stderr,"%s: Failed to fork() for opreport\n", strerror(errno));
		exit(13);
	}
	else if (pid_child_opreport == 0)
	{		
		int fd; 
		fd = open("report", O_WRONLY);
		dup2(fd,1);
		exec_opreport();
		close(fd);
	}
	
	//printf("PID %ld: Started child opreport PID %ld.\n", (long)getpid(), (long)pid_child_opreport);	
	wait_pid_child_opreport = wait(&status_exit_wait_opreport);
	
	cpi_calc();
	cpu_utilization();
	
	if (wait_pid_child_opreport == -1)
	{
		fprintf(stderr,"%s: Failed waiting for opcontrol process to finish\n", strerror(errno));
		return 1;
	}
	else if (wait_pid_child_opreport != pid_child_opreport)
	{
		abort();
	}
	
	if ( WIFEXITED(status_exit_wait_opreport) )
	{	
		printf("Exited: $? = %d\n", WEXITSTATUS(status_exit_wait_opreport));
	}
	else if ( WIFSIGNALED(status_exit_wait_opreport) )
	{		
		printf("Signal : %d%s\n", WTERMSIG(status_exit_wait_opreport), WCOREDUMP(status_exit_wait_opreport) ? " With Core File. " : "");
	}
	else
	{		 
		puts("Stopped child process. \n");
	}	
	
}


static void handler(int signo)
{
	int e = errno;	
	char *signame = "<<<SIGALRM>>>\n";	
	opreport_generate();	
	errno = e;
}

int main (int argc, char **argv)
{
	int z;
	struct sigaction new_sigalrm;
	struct itimerval real_timer;
	struct itimerval timer_values;
	
	new_sigalrm.sa_handler = handler;
	sigemptyset(&new_sigalrm.sa_mask);
	new_sigalrm.sa_flags = 0;
	
	sigaction(SIGALRM, &new_sigalrm, NULL);
	
	real_timer.it_interval.tv_sec = 2;
	real_timer.it_interval.tv_usec = 0; //2.0 seconds
	real_timer.it_value.tv_sec = 2;
	real_timer.it_value.tv_usec = 0;
	
	puts("CPI TOTCYC TOTINST dCYC dINST DSCHRGRATE avgDSCHRGRATE CAPACITY dCAPACITY BATTDSCHRG VOLT avgVOLT avgCURRENT avgCURRENTxCPI BDxCPI avgRATExCPI BUSYTIME TOTALTIME UTIL FREQ CORE0TEMP CORE1TEMP TIME\n");	
	
	z = setitimer(ITIMER_REAL, &real_timer, NULL);
	do { } while(1);
	
	
	return 0;
}
	
