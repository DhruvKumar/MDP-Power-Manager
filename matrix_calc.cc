  /*
    This program calculates the Transition Probability matrices for the MDP formulation.
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
#include <float.h>

#define MAXLINE 8192
//#define DEBUG
char STAT_FILE_PATH[]="/home/dhruv/thesis/programs/stats/2sec_sampling/past_pering_newstats_report";
using namespace std;


/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}


/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
} 

int divide_states(double maxval, double minval, int numstates, double * statearray)
{
	#ifdef DEBUG
	printf("\nOn entering divide_state values are : %lf %lf %d\n", maxval, minval, numstates);
	#endif
	int i=0;
	double current_val = minval;
	double final_val = maxval;
	double step = (maxval - minval)/(double)numstates;
	while (current_val <= final_val )
	{
		*(statearray + i) = current_val;
		current_val += step;
		i++;	
	}
	if(statearray[numstates] != maxval)
		statearray[numstates] = maxval;
	if(statearray[0] != minval)
		statearray[0] = minval;
	return 0;
	
}

int state_transition_encode(double statearray[], int numstates, double inival, double finval, char * transitionstring)
{
	#ifdef DEBUG
	puts("\nEntering state_transition_decode function\n");
	puts("Initial transition string: ");
	puts(transitionstring);
	#endif
	
	int i, transition, ini_state_number, fin_state_number;
	char ini_state[100], fin_state[100], statestring[200];
	for (i=0; i<numstates; i++)
	{		
		if (inival >= statearray[i] && inival <= statearray[i+1])
			ini_state_number = i;
		if (finval >= statearray[i] && finval <= statearray[i+1])
			fin_state_number = i;			
	}
	
	#ifdef DEBUG
	puts("\nInteger value of initial state= ");
	printf("%d", ini_state_number);
	#endif
	
	itoa(ini_state_number, ini_state);
	
	#ifdef DEBUG
	puts("\nInitial state as identified: ");
	puts(ini_state);
	#endif	
		
	itoa(fin_state_number, fin_state);
	#ifdef DEBUG
	puts("\nFinal state as identified: ");
	puts(fin_state);
	puts("\nAdding space at the end of the Initial state string: ");
	#endif 
	
	strcat(ini_state," ");
	
	#ifdef DEBUG
	puts(ini_state);
	puts("\nConcatenating initial state string with final state string and copying into statestring object: ");
	#endif
	
	
	strcpy(statestring,strcat(ini_state,fin_state));
	#ifdef DEBUG
	puts(statestring);
	cout<<endl;
	puts("\nCopying statestring to final transitionstring: ");
	#endif
	strcpy(transitionstring, statestring);
	
	#ifdef DEBUG
	puts(transitionstring);
	cout<< endl;
	#endif
	return 0;
	

	
}

int action_encode(long inifreq, long finfreq)
{
	#ifdef DEBUG
	puts("\nEntering the action encode function");
	printf("\nThe inifreq and finfreq values received are %ld, %ld", inifreq, finfreq);
	#endif
	int action;
	if(inifreq == 1000000 && finfreq == 1000000)
		action = 0;
	else if (inifreq == 1000000 && finfreq == 1333000)
		action = 1;
	else if (inifreq == 1000000 && finfreq == 1667000)
		action =2;
	else if (inifreq == 1000000 && finfreq == 2000000)
		action = 3;
	else if (inifreq == 1333000 && finfreq == 1000000)
		action =0;
	else if (inifreq == 1333000 && finfreq == 1333000)
		action = 1;
	else if (inifreq == 1333000 && finfreq == 1667000)
		action =2;
	else if (inifreq == 1333000 && finfreq == 2000000)
		action = 3;
	else if (inifreq == 1667000 && finfreq == 1000000)
		action = 0;
	else if (inifreq == 1667000 && finfreq == 1333000)
		action = 1;
	else if (inifreq == 1667000 && finfreq == 1667000)
		action = 2;
	else if (inifreq == 1667000 && finfreq == 2000000)
		action = 3;
	else if (inifreq == 2000000 && finfreq == 1000000)
		action = 0;
	else if (inifreq == 2000000 && finfreq == 1333000)
		action = 1;
	else if (inifreq == 2000000 && finfreq == 1667000)
		action = 2;
	else if (inifreq == 2000000 && finfreq == 2000000)
		action = 3;
	else action = -1;
	
	#ifdef DEBUG
	printf("\nThe action identified with this transition is %d\n", action);
	#endif
	return action;
} 



int main(int argc, char **argv)
{
	
	int TOTALSTATES = 10;
	int TOTALACTIONS = 4;
	double statevector[TOTALSTATES];
	double initial_val = 0.0, final_val = 0.0;
	double transitionProbMatrix[TOTALACTIONS][TOTALSTATES][TOTALSTATES];
	char transition_string[200];
	//end test
	double initialprob =  1.0/(double)(TOTALSTATES);
	double incrementprob = 1.0/((double)(TOTALACTIONS*TOTALSTATES*TOTALSTATES));
	double decrementprob = incrementprob/(TOTALSTATES-1);
	
	printf("\nIncrement Prob = %lf Decrement Prob = %lf\n", incrementprob, decrementprob);

	
	int j,k,l;
	for (j=0; j <TOTALACTIONS ; j++)
		{
			for (k=0 ; k<TOTALSTATES; k++)
			{
				for (l=0; l <= TOTALSTATES; l++)
				transitionProbMatrix[j][k][l] = initialprob; 
			}
		}
		
	
	char line_stat_file[MAXLINE];	
	FILE *fp;
	char * pch;
	fp = fopen(STAT_FILE_PATH, "r");
	double linecount = 0.0;
	int inifreqindex, changefreq, finfreqindex, core0temp, core1temp ;
	long  rate, busytime, totaltime, FREQ;
	double cpi, volt, avgvolt, avgcurrent, AVGCURRENTCPI,BDCPI, AVGRATECPI, totcyc, totinst, dcyc, dinst, cap, dcap, maxBDCPI=0, maxAVGRATECPI=0, maxAVGCURRENTCPI=0, minAVGCURRENTCPI=DBL_MAX, minBDCPI=DBL_MAX, minAVGRATECPI=DBL_MAX, util, avgrate, dschrg;
	
	
	if(fp != NULL)
	{
		
		while(fgets(line_stat_file, MAXLINE, fp)!=NULL)
			{
				linecount ++;
				cout << endl;
				int match=0;	
					
	    			
	    			pch = strtok(line_stat_file," ");
	    			
	    			while(pch != NULL)
	    			{
	    				match++;
	    				switch(match)
	    				{
	    					case 1: cpi = atof(pch); break;
	    					case 2: totcyc = atof(pch); break;
	    					case 3: totinst = atof(pch); break;
	    					case 4: dcyc = atof(pch); break;
	    					case 5: dinst = atof(pch); break;
	    					case 6: rate = atol(pch); break;
	    					case 7: avgrate = atof(pch); break;
	    					case 8: cap = atof(pch); break;
	    					case 9: dcap = atof(pch); break;
	    					case 10: dschrg = atof(pch); break;
	    					case 11: volt = atof(pch); break;
	    					case 12: avgvolt = atof(pch); break;
	    					case 13: avgcurrent = atof(pch); break;
	    					case 14: AVGCURRENTCPI = atof(pch); break;
	    					case 15: BDCPI = atof(pch); break;
	    					case 16: AVGRATECPI = atof(pch); break;
	    					case 17: busytime = atol(pch); break;
	    					case 18: totaltime = atol(pch); break;
	    					case 19: util = atof(pch); break;
	    					case 20: FREQ = atol(pch); break;
	    					case 21: inifreqindex = atoi(pch); break;
	    					case 22: changefreq = atoi(pch); break;
	    					case 23: finfreqindex = atoi(pch); break;
	    					case 24: core0temp = atoi(pch); break;
	    					case 25: core1temp = atoi(pch); break;
	    					default: break; 
	    				}
	    				
	    				pch = strtok(NULL," ");
	    				
	    			}
	    			
	    			if(BDCPI > maxBDCPI) 
	    				maxBDCPI=BDCPI;
	    			if(AVGRATECPI > maxAVGRATECPI)
	    				maxAVGRATECPI = AVGRATECPI;
	    			if(AVGCURRENTCPI > maxAVGCURRENTCPI)
	    				maxAVGCURRENTCPI = AVGCURRENTCPI;
	    			if(BDCPI < minBDCPI)
	    				minBDCPI = BDCPI;
	    			if(AVGRATECPI < minAVGRATECPI)
	    				minAVGRATECPI = AVGRATECPI;
	    			if(AVGCURRENTCPI < minAVGCURRENTCPI)
	    				minAVGCURRENTCPI = AVGCURRENTCPI;
	    			
	    		} 
	    		
	}
	
	//printf("\n\n MAX_BDCPI = %lf MIN_BDCPI = %lf MAX_AVGRATECPI = %lf MIN_AVGRATECPI = %lf\n\n", maxBDCPI, minBDCPI, maxAVGRATECPI, minAVGRATECPI);
	
	//divide_states(maxBDCPI, minBDCPI, TOTALSTATES, statevector);
	
	// For AVGCURRENTCPI
	
	divide_states(maxAVGCURRENTCPI, minAVGCURRENTCPI, TOTALSTATES, statevector);
	
	#ifdef DEBUG
	for(int i=0; i <= TOTALSTATES; i++)
		printf("%lf\n", statevector[i]);	
	state_transition_encode(statevector, TOTALSTATES, initial_val, final_val, transition_string);
	puts("\nFinal transition string in the main function: ");
	puts(transition_string);
	cout<<endl<<endl;
	#endif
	
	fclose(fp);
	fp = fopen(STAT_FILE_PATH, "r");
	linecount=0.0;
	double prev_AVGRATECPI, prev_BDCPI, prev_FREQ=FREQ, prev_AVGCURRENTCPI;
	int flag = 0, action;
	while(fgets(line_stat_file, MAXLINE, fp)!=NULL)
			{
				linecount ++;
				cout << endl;
				int match=0;	
					
	    			
	    			pch = strtok(line_stat_file," ");
	    			
	    			while(pch != NULL)
	    			{
	    				match++;
	    				switch(match)
	    				{
	    					case 1: cpi = atof(pch); break;
	    					case 2: totcyc = atof(pch); break;
	    					case 3: totinst = atof(pch); break;
	    					case 4: dcyc = atof(pch); break;
	    					case 5: dinst = atof(pch); break;
	    					case 6: rate = atol(pch); break;
	    					case 7: avgrate = atof(pch); break;
	    					case 8: cap = atof(pch); break;
	    					case 9: dcap = atof(pch); break;
	    					case 10: dschrg = atof(pch); break;
	    					case 11: volt = atof(pch); break;
	    					case 12: avgvolt = atof(pch); break;
	    					case 13: avgcurrent = atof(pch); break;
	    					case 14: AVGCURRENTCPI = atof(pch); break;
	    					case 15: BDCPI = atof(pch); break;
	    					case 16: AVGRATECPI = atof(pch); break;
	    					case 17: busytime = atol(pch); break;
	    					case 18: totaltime = atol(pch); break;
	    					case 19: util = atof(pch); break;
	    					case 20: FREQ = atol(pch); break;
	    					case 21: inifreqindex = atoi(pch); break;
	    					case 22: changefreq = atoi(pch); break;
	    					case 23: finfreqindex = atoi(pch); break;
	    					case 24: core0temp = atoi(pch); break;
	    					case 25: core1temp = atoi(pch); break;
	    					default: break; 
	    				}
	    				
	    				pch = strtok(NULL," ");
	    				
	    			}
	    		
		    		
				
					if(flag!=0 && action !=-1)
					{
						
						state_transition_encode(statevector, TOTALSTATES, prev_AVGCURRENTCPI, AVGCURRENTCPI, transition_string);
						action = action_encode(prev_FREQ, FREQ) ;
						
						int INISTATE=0, FINSTATE=0;
						
					
						sscanf(transition_string,"%d %d", &INISTATE, &FINSTATE);
						printf("\nInitial State = %d Final State = %d Action =  %d\n", INISTATE, FINSTATE, action);
						transitionProbMatrix[action][INISTATE][FINSTATE] += incrementprob;
						
						k=INISTATE;
						l=0;
						while(l<TOTALSTATES)
						{
						  if(transitionProbMatrix[action][k][l]>0 && l != FINSTATE)
						  {
						 
						  	transitionProbMatrix[action][k][l] -= decrementprob;
						  	printf("\nDecrementing %d %d %d\n",action,k,l);
						  }
						  l++;
						 }
						
					}
					
					
					
						
					
				
	    		prev_AVGCURRENTCPI = AVGCURRENTCPI;	
	    		prev_AVGRATECPI = AVGRATECPI;
	    		prev_BDCPI = BDCPI;
	    		prev_FREQ = FREQ;
	    		flag +=1;
	    			
	    			
	    		}	    		
	 
	fclose(fp);
	double observationprobability = 1.0/(double)linecount;
	 
	int statenumber = 0;
	for (j=0; j <= 3 ; j++)
		{
			printf("\n\n\n------Matrix for action %d------\n\n", j);
			for (k=0 ; k<= TOTALSTATES-1; k++)
				{ 
					
					for (l=0; l <= TOTALSTATES-1; l++)
						{
							 
							printf("%lf", transitionProbMatrix[j][k][l]);
							printf(" ");
							
								
						}
					 
					printf(";\n");
				}
		}
	
	int  reward_distribution = (TOTALSTATES/2);
	for(j=0; j< 4; j++)
		{
			printf("\n\n------Reward Matrix for action %d------\n\n", j);
			for(k=0; k<TOTALSTATES; k++)
			{
				for(l=0; l<TOTALSTATES; l++)
				{
					printf("%d", reward_distribution);
					printf(" ");
					reward_distribution--;
				}
				printf(";\n");
				reward_distribution = (TOTALSTATES/2);
			}
		}
	
	printf("\n\nState Vector\n");
	
	for(k=0; k<=TOTALSTATES;k++)
	{
		
		printf("\n%lf",statevector[k]);
	}
	printf("\n\nmaxAVGCURRENTCPI= %lf  minAVGCURRENTCPI=%lf", maxAVGCURRENTCPI, minAVGCURRENTCPI);
	cout<<endl<<endl<<endl;
	return 0;
}






















