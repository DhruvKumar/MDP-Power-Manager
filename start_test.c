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
#include <inotifytools/inotifytools.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <unistd.h>

/*
 
Compile with gcc -linotifytools -o program program.c
Command line arguments: 
argv[1] 	test executable name : past, mdp, avgn, maxperf, maxlife     
argv[2]		file to store statistics generated by the DVS program
argv[3]		name of the benchmark from phoronix test suite (will run in batch mode)


mdptest.xml located in /root/.phoronix-test-suite/test-suites
*/

const char ACPI_GOVERNOR_FILE[] = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
const char OPCONTROL_EXEC_PATH[] =  "/usr/local/bin/opcontrol";
const char PTS_EXEC_PATH[]= "/usr/bin/phoronix-test-suite";
extern char **environ;
 


void write_line(const char * filename, const char *fmt, ...)
{
	 FILE * fp = fopen(filename, "w+"); 
	 if(!fp) puts("\nCould not open file for writing\n");  
	 va_list ap;
	 // get variable argument list passed
	 va_start(ap, fmt);
	 vfprintf(fp, fmt, ap);        
	 va_end(ap);
	 fclose(fp);
}

static void exec_opcontrol(void)
{
	static char * argv[] = {"opcontrol", "--start", NULL}; 	
	execve(OPCONTROL_EXEC_PATH, argv, environ);	
	fprintf(stderr, "%s: execve(2)\n", strerror(errno));
} 

void start_oprofile()
{
	pid_t pid_child_opcontrol;
	pid_t wait_pid_child_opcontrol;
	int status_exit_wait_opcontrol;
			
	pid_child_opcontrol = fork();
	
	if(pid_child_opcontrol == -1)
	{
		fprintf(stderr,"%s: Failed to fork() for opcontrol\n", strerror(errno));
		exit(13);
	}
	else if (pid_child_opcontrol == 0)
	{		
		exec_opcontrol();		
	}
	
	
	wait_pid_child_opcontrol = wait(&status_exit_wait_opcontrol);
	
	
	
	if (wait_pid_child_opcontrol == -1)
	{
		fprintf(stderr,"%s: Failed waiting for opcontrol process to finish\n", strerror(errno));
		return 1;
	}
	else if (wait_pid_child_opcontrol != pid_child_opcontrol)
	{
		abort();
	}
	
	if ( WIFEXITED(status_exit_wait_opcontrol))
	{	
		printf("Exited: $? = %d\n", WEXITSTATUS(status_exit_wait_opcontrol));
	}
	else if ( WIFSIGNALED(status_exit_wait_opcontrol) )
	{		
		printf("Signal : %d%s\n", WTERMSIG(status_exit_wait_opcontrol), WCOREDUMP(status_exit_wait_opcontrol) ? " With Core File. " : "");
	}
	else
	{		 
		puts("Stopped child process. \n");
	}	
}

static void exec_dvsprogram(const char * dvsprogram, const char * options)
{
	//char resultsdir[100];
	//strcat(resultsdir, ">");
	//strcat(resultsdir, options);
	int fd = open(options, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	//close(1);
	dup2(fd, 1);
	char * argv[] = {dvsprogram, NULL}; 	
	execve(dvsprogram, argv, environ);	
	fprintf(stderr, "%s: execve(2)\n", strerror(errno));
	close(fd);
} 


void start_dvs_program(const char * dvsprogram, const char * options)
{
	pid_t pid_child_dvsprogram, wait_pid_child_dvsprogram;
	int status_exit_wait_dvsprogram, fd;
			
	pid_child_dvsprogram = fork();
	
	if(pid_child_dvsprogram == -1)
	{
		fprintf(stderr,"%s: Failed to fork() for dvsprogram\n", strerror(errno));
		exit(13);
	}
	else if (pid_child_dvsprogram == 0)
	{		
		
		//fd = open(options, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		//dup2(fd,1);
		exec_dvsprogram(dvsprogram, options);
		//close(fd);		
	}
	
	
}

void exec_pts(char * benchmarkname)
{
	char * argv[] = {"phoronix-test-suite","batch-run", benchmarkname, NULL}; 	
	execve(PTS_EXEC_PATH, argv, environ);	
	fprintf(stderr, "%s: execve(2)\n", strerror(errno));
}

void start_pts(char * benchmarkname)
{
	pid_t pid_child_pts;			
	pid_child_pts = fork();	
	if(pid_child_pts == -1)
	{
		fprintf(stderr,"%s: Failed to fork() for opcontrol\n", strerror(errno));
		exit(13);
	}
	else if (pid_child_pts == 0)
	{		
		//export_pts_env_var(test_results_name, test_results_identifier);
		exec_pts(benchmarkname);		
	}	
}
 
int main(int argc, char ** argv) 
{   
               
        // initialize and watch the entire directory tree from the current working
        // directory downwards for all events
        if ( !inotifytools_initialize() || !inotifytools_watch_recursively( "/proc/acpi/ac_adapter/AC/", IN_ALL_EVENTS ) )
        {
                fprintf(stderr, "%s\n", strerror( inotifytools_error() ) );
                return -1;
        }
        // set time format to 24 hour time, HH:MM:SS
        inotifytools_set_printf_timefmt( "%T" );
        // Output all events as "<timestamp> <path> <events>"
        struct inotify_event * event = inotifytools_next_event( -1 );
        if ( event ) 
        {
                inotifytools_printf( event, "%T %w%f %e\n" );
                event = inotifytools_next_event( -1 );
        }        
        // Configure userspace as the scaling governor in ACPI 
        write_line(ACPI_GOVERNOR_FILE, "userspace");       
        // Start oprofile 
        start_oprofile();       
        // Start past/mdp/avgn/maxperf/maxlife        
        // Start phoronix-test-suite benchmark mdp     
        start_pts(argv[3]);
        start_dvs_program(argv[1], argv[2]); 
}
