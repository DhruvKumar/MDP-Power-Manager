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
#include <signal.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <time.h>  
#include <sys/time.h>  
#include <sys/types.h> 
#include <sys/wait.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <errno.h>  
#include <stdarg.h>  
#include <unistd.h>  
#include <limits.h>
#include <sys/mman.h>
#include <string.h>

extern char **environ;
#define ptt_binary "/home/dhruv/Dpiperf/bin/ptt"

#define LOGFILE "/home/dhruv/thesis/programs/defense_programs/logfile"
#define PTT_DUMP "/home/dhruv/thesis/programs/defense_programs/ptt_dump"
#define BATTERY "/proc/acpi/battery/BAT0/state"
#define UTILIZATION "/proc/stat"
#define TEMPERATURE0 "/proc/acpi/thermal_zone/THM0/temperature"
#define TEMPERATURE1 "/proc/acpi/thermal_zone/THM1/temperature"
#define FREQUENCYREAD "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define FREQUENCYWRITE "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed"
#define LOWTHRESHOLD 0.50
#define HIGHTHRESHOLD 0.70
#define REDIRECT "/home/dhruv/thesis/programs/defense_programs/screen_dump"
/* Static global variables */

unsigned long prev_total_time, prev_idle_time, prev_rate;
int prev_capacity = 56160; /* Designed capacity of Li Ion battery on this computer */
int frequencies[] = {1000000, 1333000, 1667000, 2000000};


struct logdata
{
  int line_num;
  char *dominant_pid_number;
  char *dominant_pid_name;
  char *instructions;
  char *cycles;
  char *cpi;
  double utilization;
  int initial_frequency;
  int final_frequency;
  int action;
  int cap;
  int dcap;
  double inspcap;
  int rate;
  int voltage;
  double current;
  int temp0;
  int temp1;
};

struct logdata log;

static void exec_ptt_noints(void)
{
  static char *argv[] = {"ptt", "noints", NULL};
  execve(ptt_binary, argv, environ);
}

static void exec_ptt_noautoterm(void)
{
  static char *argv[] = {"ptt", "noautoterm", NULL};
  execve(ptt_binary, argv, environ);
}

static void exec_ptt_init(void)  
{
  static char *argv[] = {"ptt", "init", "instr", "cycles", NULL};
  execve(ptt_binary, argv, environ);
}

void get_temperature_info()
{
  char dummy1[20], dummy2[5];
  int temp_0, temp_1;
  FILE *temp0_fp = fopen(TEMPERATURE0, "r");
  FILE *temp1_fp = fopen(TEMPERATURE1, "r");

  if (temp0_fp == NULL)
    {
      perror("can't open CPU0 temperature file");
      exit(-1);
    }
  if (temp1_fp == NULL)
    {
      perror("can't open CPU1 temperature file");
      exit(-1);
    }

  fscanf(temp0_fp ,"%s %d %s", &dummy1, &temp_0, &dummy2);
  fscanf(temp1_fp ,"%s %d %s", &dummy1, &temp_1, &dummy2);
  log.temp0 = temp_0;
  fclose(temp0_fp);
  log.temp1 = temp_1;
  printf("\n\ncpu 0 temperature = %d\tcpu 1 temperature = %d", temp_0, temp_1);
  fclose(temp1_fp);
}


double get_cpu_utilization()
{  
  FILE *stat_fp = fopen(UTILIZATION, "r");
  if(stat_fp == NULL)
    {
      perror(" can't open /proc/stat file for reading");
      exit(-1);
    }
  int nbytes = 100;
  char * stat;
  char line[400];
  stat =  (char *)malloc(nbytes +1);
  getline(&stat, &nbytes, stat_fp);  
  char cpuname[10];
  unsigned long int user, nice, system, idle, iowait, irq, softirq, steal, guest;
  unsigned long total_time, interval_total_time, interval_idle_time;
  double interval_utilization;
  sscanf(stat, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld", &cpuname, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
  total_time = user + system +idle;  
  interval_total_time = total_time - prev_total_time;
  interval_idle_time = idle - prev_idle_time;  
  interval_utilization = (double)(interval_total_time - interval_idle_time)/interval_total_time;
  prev_total_time = total_time;
  prev_idle_time = idle;
  #ifdef DEBUG
   printf("%s %ld %ld %ld %ld %ld %ld %ld %ld %ld", cpuname, user, nice, system, idle, iowait, irq, softirq, steal, guest);
   printf("\nprev_total_time = %ld\nprev_idle_time = %ld\nidle = %ld\ntotal_time = %ld\ninterval_total_time = %ld\ninterval_idle_time = %ld\ninterval_utilization = %lf\n", total_time, prev_idle_time, idle, total_time, interval_total_time, interval_idle_time, interval_utilization);
  #endif
   
   printf("\n\ncpu utilization = %lf", interval_utilization);
   free(stat);
   fclose(stat_fp);
   return interval_utilization; 
  
}

int initialize_ptt()
{
  int status;
  pid_t pid = vfork();
  if(pid == 0)
    {
      exec_ptt_noautoterm();
    } 
  if (wait(&status) != pid)
    perror("wait error");

  if( (pid = vfork()) == 0)
    {
      exec_ptt_noints();
    }
  if (wait(&status) != pid)
    perror("wait error");
  
  pid = fork();
  if(pid == 0)
    {
      exec_ptt_init();
    }
  if (wait(&status) != pid)
    perror("wait error");
    else return(0);
  
}

int get_current_frequency_index()
{
  FILE *freq_fp = fopen(FREQUENCYREAD, "r");
  int currentfrequency;

  if(freq_fp == NULL)
    {
      perror(" can't open /sys.../scaling_cur_frequency file for reading");
      exit(-1);
    }
  int nbytes = 100;
  char * stat;
  char line[400];
  stat =  (char *)malloc(nbytes +1);
  getline(&stat, &nbytes, freq_fp);

  sscanf(stat, "%d", &currentfrequency);

  if (currentfrequency == 1000000)
    {
      fclose(freq_fp);  
      return 0;
    }
  else if (currentfrequency == 1333000)
    {
      fclose(freq_fp);
      return 1;
    }
  else if (currentfrequency == 1667000)
    {
      fclose(freq_fp);
      return 2;
    }
  else if (currentfrequency == 2000000)
    {
      fclose(freq_fp);
      return 3;
    }
 
}

int perform_dvs_past(int freqindex, double utilization)
{
  int targetfreqindex;
  if (utilization < LOWTHRESHOLD && freqindex ==0)
    targetfreqindex = freqindex;
  else if (utilization < LOWTHRESHOLD && freqindex != 0 && freqindex >= 1)
    targetfreqindex = freqindex -1;
  else if (utilization > HIGHTHRESHOLD && freqindex == 3)
    targetfreqindex = freqindex;
  else if (utilization > HIGHTHRESHOLD && freqindex != 3 && freqindex <= 2)
    targetfreqindex = freqindex + 1;
  else targetfreqindex = freqindex;
  
  FILE *writefreq_fp = fopen(FREQUENCYWRITE, "w+");
  if(writefreq_fp == NULL)
    {
      perror(" can't open /sys.../scaling_setspeed  file for reading");
      exit(-1);
    }
  switch (targetfreqindex)
    {
    case 0:
      fprintf(writefreq_fp, "%s", "1000000");
      break;
    case 1:
      fprintf(writefreq_fp, "%s", "1333000");
      break;
    case 2:
      fprintf(writefreq_fp, "%s", "1667000");
      break;
    case 3:
      fprintf(writefreq_fp, "%s", "2000000");
      break;

    }
  
  fclose(writefreq_fp);
  return targetfreqindex;
}

void get_battery_info()
{
  FILE *battery_fp = fopen(BATTERY, "r");
  if(battery_fp == NULL)
    {
      perror("can't open /proc/acpi/battery.. file");
      exit(-1);
    }

  int nbytes = 300;
  char *battery_info_line;
  int linecount = 0;
  int rate, capacity, voltage;
  char dummy1[15], dummy2[15], dummy3[15];

  battery_info_line =  (char *)malloc(nbytes +1);  

  while(getline(&battery_info_line, &nbytes, battery_fp) > 0)
    {
      linecount++;
      if (linecount == 4)
	{
	  /* present discharge rate line */
	   sscanf(battery_info_line, "%s %s %d %s", &dummy1, &dummy2, &rate, &dummy3);  
	}
      else if (linecount == 5)
	{
	  /* remaining capacity line */
	  sscanf(battery_info_line, "%s %s %d %s", &dummy1, &dummy2, &capacity, &dummy3);	  
	}
      else if (linecount == 6)
	{
	  /* present voltage line */
	  sscanf(battery_info_line, "%s %s %d %s", &dummy1, &dummy2, &voltage, &dummy3);	  
	}
    }
  
  log.cap = capacity;
  log.rate = rate;
  log.voltage = voltage;
  log.current = (double)rate/voltage;
  if((prev_capacity - capacity) !=0)
    {
      log.dcap = prev_capacity - capacity;
      log.inspcap = (double)atol(log.instructions)/log.dcap;
      prev_capacity = capacity;
    }
  else
    {
      /* handle infinity */
      log.dcap = 0;
      log.inspcap = UINT_MAX;
      prev_capacity = capacity;
    }
  printf("\n\n rate = %d remaining_capacity = %d voltage = %d ", rate, capacity, voltage);
  free(battery_info_line);
  fclose(battery_fp);
}


static void handler(int signo)
{
   
  int status;
  pid_t pid;
  double cpu_utilization;
 
  #ifdef DEBUG
  FILE *fp_logfile = fopen(LOGFILE, "a");
  #endif
  
  if( (pid = vfork()) < 0)
    perror("fork error at ptt dump");
  else if (pid == 0)
    {
      //child
      static char *argv[] = {"ptt", "dump", "-cpi", "-f", PTT_DUMP,  NULL};
      execve(ptt_binary, argv, environ);
    }
  else if (pid > 0)
    {
      //PARENT
      
      if (wait(&status) != pid)
	perror("wait on child error");

      int fd_redirect_out = open(REDIRECT, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );

      dup2(fd_redirect_out, 1);
	
      int fd_ptt_dump = open(PTT_DUMP, O_RDONLY);
      struct stat struct_ptt_dump;
      fstat(fd_ptt_dump, &struct_ptt_dump);

      #ifdef DEBUG
      printf("\n\n\t\t\t --- The size of the dump is = %d -----\n\n", struct_ptt_dump.st_size);
      #endif
      
      char *membuffer;
      char pid_num[100];
      char pid_name[100];
      char cycles[100], instr[100], cpi[100] ;
      int pid_index =0;
      int cycles_index = 0;
      int instr_index = 0;
      int cpi_index = 0 ;
      int len = (int)struct_ptt_dump.st_size;
      int newline_count = 0;
      int n = 0;
      
      if( (membuffer = mmap(0, struct_ptt_dump.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd_ptt_dump, 0)) == (caddr_t) -1)
       perror("mmap error");

      /* skip newlines and go to the pid line */
      while (newline_count != 5)
       if ( membuffer[n++] == '\n' )
        newline_count++ ;

      /* find first nonspace character */
      while ( membuffer[n] == ' ' )
       n++;

      while ( membuffer[n] != ' ')
	pid_num[pid_index++] = membuffer[n++] ;
            
      pid_num[pid_index] = '\0';

      /* assign struct */
      log.dominant_pid_number = pid_num;

      while ( membuffer[len] == '\n' )
	len--;

      len-=3;

      while ( membuffer[len] != ' ' )
	cpi[cpi_index++] = membuffer[len--];

      cpi[cpi_index] = '\0';
      
      //reverse cpi
      int i,j, k;
      for( i=0, j= cpi_index-1 ; i<j ; i++, j-- )
	{
	  k = cpi[i];
	  cpi[i] = cpi[j];
	  cpi[j] = k;
	}

      /* assign struct */

      log.cpi = cpi;

      while ( membuffer[len] == ' ' )
        len--;

      while ( membuffer[len] != ' ')
	cycles[cycles_index++] = membuffer[len--];

      cycles[cycles_index] = '\0';

      //reverse cycles

      for( i=0, j=cycles_index-1; i<j ; i++, j--)
	{
	  k = cycles[i];
	  cycles[i] = cycles[j];
	  cycles[j] = k;
	}

      log.cycles = cycles;

      while ( membuffer[len] == ' ' )
        len--;

      while ( membuffer[len] != ' ')
	instr[instr_index++] = membuffer[len--];

      instr[instr_index] = '\0';

      //reverese instructions

      for( i=0, j=instr_index-1; i<j ; i++, j--)
	{
	  k = instr[i];
	  instr[i] = instr[j];
	  instr[j] = k;
	}

      log.instructions = instr;

      close(fd_ptt_dump);
      
      k=0;
      char proc_file[40];
      proc_file[k++] = '/';
      proc_file[k++] = 'p';
      proc_file[k++] = 'r';
      proc_file[k++] = 'o';
      proc_file[k++] = 'c';
      proc_file[k++] = '/';      
      
      for( i= 0;  pid_num[i] != '\0' ; i++ )
	proc_file[k++] = pid_num[i];

      proc_file[k] = '\0';

      strcat(proc_file, "/stat");

      FILE *proc_fp = fopen(proc_file, "r");

      if(proc_fp != NULL)
	{
	  int nbytes = 100;
	  char *stat_output;
	  stat_output =  (char *)malloc(nbytes +1);
	  if(getline(&stat_output, &nbytes, proc_fp) > 0)
	   {
	    k=0;
	    i=0;
	    while( (char *)stat_output[k++] != '(' )
	      ;     
	    while( (char *)stat_output[k] != ')' )
	     pid_name[i++] =  (char *)stat_output[k++];
	    pid_name[i] = '\0';
	    log.dominant_pid_name = pid_name;
	    fclose(proc_fp);
	   }
	   else
	   {
	     log.dominant_pid_name = "undefined";
	     fclose(proc_fp);
	   }
	   free(stat_output);
	}
      else
	{
	  log.dominant_pid_name = "undefined";
	}
      

      char currentfrequency[10], finalfrequency[10];
      cpu_utilization = get_cpu_utilization();

      log.utilization = cpu_utilization;

      get_battery_info();

      int freq_index = get_current_frequency_index();

      log.initial_frequency = frequencies[freq_index];

      printf("\n\ncurrent frequency = %d", frequencies[freq_index]);

      int dvs_freqindex = perform_dvs_past(freq_index, cpu_utilization);

      printf("\n\n DVS calculated freq index = %d", dvs_freqindex);

      freq_index = get_current_frequency_index();

      log.final_frequency = frequencies[freq_index];

      log.action = freq_index;

      printf("\tfinal frequency = %d", frequencies[freq_index]);   

      get_temperature_info();
      
      printf("\n\n pid = %s (%s) cpi = %s cycles = %s instruction = %s ", pid_num, pid_name, cpi, cycles, instr);
      
      #ifdef DEBUG
       puts(stat_output);    
       puts("\n\n extracted dominant pid name: ");
       puts(pid_name);       
       for ( n =0; n < struct_ptt_dump.st_size ; n++)      
        fprintf(fp_logfile,"%c", membuffer[n]);
       fflush(fp_logfile); 
      #endif
       
      fflush(stdout);
     

      FILE *logfile_fp = fopen(LOGFILE, "a");

      if (logfile_fp == NULL)
	{
	  perror("can't open or create logfile");
	  exit(-1);
	}

      /* Print contents of structure log */

      fprintf(logfile_fp,"\n%d %s %s %s %s %s %lf %d %d %d %d %d %lf %d %d %lf %d %d", ++log.line_num, log.dominant_pid_number, log.dominant_pid_name, log.instructions, log.cycles, log.cpi, log.utilization, log.initial_frequency, log.final_frequency, log.action, log.cap, log.dcap, log.inspcap, log.rate, log.voltage, log.current, log.temp0, log.temp1);

      fclose(logfile_fp);
      
      if ( munmap(membuffer, struct_ptt_dump.st_size) < 0)
	{
	  perror(" mem unmap failed ");
	  exit(-1);
	}
      
      system("ptt term >> ptt_msgs");
      system("ptt noautoterm >> ptt_msgs");
      system("ptt noints >> ptt_msgs");
      system("ptt init instr cycles >> ptt_msgs");

      close(fd_redirect_out);
      
    }
  
  
}


int main()
{
  
 struct sigevent evp;
 timer_t timer;
 int ret;
 struct itimerspec ts;
 
 initialize_ptt();
 
 evp.sigev_value.sival_ptr = &timer;
 evp.sigev_notify = SIGEV_SIGNAL;
 evp.sigev_signo = SIGUSR1;
 ret = timer_create(CLOCK_REALTIME, &evp, &timer);
 
 ts.it_interval.tv_sec = 2;
 ts.it_interval.tv_nsec = 0;
 ts.it_value.tv_sec = 2;
 ts.it_value.tv_nsec = 0;

 ret = timer_settime(timer, 0 , &ts, NULL);

 
 if (signal(SIGUSR1, handler) == SIG_ERR)
   {
     fprintf(stderr, "\nCannot handle SIGUSR1\n");
     exit(EXIT_FAILURE);
   }

 
 do {} while (1);

 return 0;
 
}
