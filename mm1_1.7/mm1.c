/* External definitions for single-server queueing system. */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>  
#include <math.h>
#include "lcgrand.h"  /* Header file for random-number generator. */

#define Q_LIMIT 100  /* Limit on queue length. */
#define BUSY      1  /* Mnemonics for server's being busy */
#define IDLE      0  /* and idle. */
#define events 2000  /*total number of events*/

int   next_event_type, num_custs_delayed, num_delays_required, num_events,
      num_in_q, server_status, max_queue, cus_num_arr, cus_num_depart, num_in_system;
float area_num_in_q, area_server_status, mean_interarrival, mean_service,
      sim_time, time_arrival[Q_LIMIT + 1], time_last_event, time_next_event[3],
      total_of_delays, max_delay, arr_list[events + 1], depart_list[events + 1], max_t, excess1, area_num_in_sys, total_t;
FILE  *infile, *outfile;

void  initialize(void);
void  timing(void);
void  arrive(void);
void  depart(void);
void  report(void);
void  update_time_avg_stats(void);
void max_time(void);
float expon(float mean);


int main()  /* Main function. */
{
    /* Open input and output files. */

    infile  = fopen("mm1.in",  "r");
    outfile = fopen("mm1.out", "w");

    /* Specify the number of events for the timing function. */

    num_events = 2;

    /* Read input parameters. */

    fscanf(infile, "%f %f %d", &mean_interarrival, &mean_service,
           &num_delays_required);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Single-server queueing system\n\n");
    fprintf(outfile, "Mean interarrival time%11.3f minutes\n\n",
            mean_interarrival);
    fprintf(outfile, "Mean service time%16.3f minutes\n\n", mean_service);
    fprintf(outfile, "Number of customers%14d\n\n", num_delays_required);

    /* Initialize the simulation. */

    initialize();

    /* Run the simulation while more delays are still needed. */

    while (num_custs_delayed < num_delays_required)     
    {
        /* Determine the next event. */

        timing();

        /* Update time-average statistical accumulators. */

        update_time_avg_stats();

        /* Invoke the appropriate event function. */

        switch (next_event_type) 
        {
            case 1:
                arrive();
                break;
            case 2:
                depart();
                break;
        }
    }

    /*최대 시간이 걸린 고객시간*/
    max_time();

    /* Invoke the report generator and end the simulation. */

    report();

    fclose(infile);
    fclose(outfile);

    return 0;
}



void initialize(void)  /* Initialization function. */
{
    /* Initialize the simulation clock. */

    sim_time = 0.0;

    /* Initialize the state variables. */

    server_status   = IDLE;
    num_in_q        = 0;
    num_in_system   = 0;
    time_last_event = 0.0;


    /* Initialize the statistical counters. */

    num_custs_delayed  = 0;
    total_of_delays    = 0.0;
    area_num_in_q      = 0.0;
    area_num_in_sys    = 0.0;
    area_server_status = 0.0;
    max_queue = 0;
    max_delay = 0;
    cus_num_arr = 0;
    cus_num_depart = 0;
    max_t = 0.0;
    total_t = 0.0;
    excess1 = 0;

    /* Initialize event list.  Since no customers are present, the departure
       (service completion) event is eliminated from consideration. */

    time_next_event[1] = sim_time + expon(mean_interarrival);
    time_next_event[2] = 1.0e+30;
}


void timing(void)  /* Timing function. */
{
    int   i;
    float min_time_next_event = 1.0e+29;

    next_event_type = 0;

    /* Determine the event type of the next event to occur. */

    for (i = 1; i <= num_events; ++i)
        if (time_next_event[i] < min_time_next_event)
        {   
            min_time_next_event = time_next_event[i];
            next_event_type     = i;
        }

    /* Check to see whether the event list is empty. */

    if (next_event_type == 0)
    {
        /* The event list is empty, so stop the simulation. */

        fprintf(outfile, "\nEvent list empty at time %f", sim_time);
        exit(1);
    }

    /* The event list is not empty, so advance the simulation clock. */

    sim_time = min_time_next_event;
}


void arrive(void)  /* Arrival event function. */
{
    float delay;

    /*system num 증가*/
    ++num_in_system;

    /*총 고객 수 증가*/
    ++cus_num_arr;
    
    /* Schedule next arrival. */

    time_next_event[1] = sim_time + expon(mean_interarrival);

    /*고객 당 arrive time 저장*/
    arr_list[cus_num_arr] = sim_time;

    /* Check to see whether server is busy. */

    if (server_status == BUSY)
    {
        /* Server is busy, so increment number of customers in queue. */

        ++num_in_q;

        /*최대 queue length*/
        if (max_queue < num_in_q) {
            max_queue = num_in_q;
        }

        /* Check to see whether an overflow condition exists. */

        if (num_in_q > Q_LIMIT)
        {
            /* The queue has overflowed, so stop the simulation. */

            fprintf(outfile, "\nOverflow of the array time_arrival at");
            fprintf(outfile, " time %f", sim_time);
            exit(2);
        }

        /* There is still room in the queue, so store the time of arrival of the
           arriving customer at the (new) end of time_arrival. */

        time_arrival[num_in_q] = sim_time;
    }

    else
    {
        /* Server is idle, so arriving customer has a delay of zero.  (The
           following two statements are for program clarity and do not affect
           the results of the simulation.) */

        delay            = 0.0;
        total_of_delays += delay;

        /* Increment the number of customers delayed, and make server busy. */

        ++num_custs_delayed;
        server_status = BUSY;

        /* Schedule a departure (service completion). */

        time_next_event[2] = sim_time + expon(mean_service);
    }
}


void depart(void)  /* Departure event function. */
{
    int   i;
    float delay;

    /*system num 감소*/
    --num_in_system;

    /*departure 고객 수 체크*/
    ++cus_num_depart;

    /*고객 당 departure time 저장*/
    depart_list[cus_num_depart] = sim_time;

    /* Check to see whether the queue is empty. */

    if (num_in_q == 0)
    {
        /* The queue is empty so make the server idle and eliminate the
           departure (service completion) event from consideration. */

        server_status      = IDLE;
        time_next_event[2] = 1.0e+30;
    }

    else
    {
        /* The queue is nonempty, so decrement the number of customers in
           queue. */

        --num_in_q;

        /* Compute the delay of the customer who is beginning service and update
           the total delay accumulator. */

        delay            = sim_time - time_arrival[1];
        total_of_delays += delay;
        
        /*delay in queue가 1이상인 고객 수 체크*/
        if(delay > 1){
            ++excess1;
        }

        /*Max delay in queue*/
        if (max_delay < delay) {
            max_delay = delay;
        }

        /* Increment the number of customers delayed, and schedule departure. */

        ++num_custs_delayed;
        time_next_event[2] = sim_time + expon(mean_service);

        /* Move each customer in queue (if any) up one place. */

        for (i = 1; i <= num_in_q; ++i)
            time_arrival[i] = time_arrival[i + 1];

    }
}

void max_time(void) { /*최대시간 걸린 고객시간 생성 함수 + system에서 머문시간 저장*/
    int i;

    for (i = 1 ; i <= cus_num_depart ; i++) {

        total_t += depart_list[i] - arr_list[i];

        if (max_t < depart_list[i] - arr_list[i]) {
            max_t = depart_list[i] - arr_list[i];
        }
    }
}



void report(void)  /* Report generator function. */
{
    /* Compute and write estimates of desired measures of performance. */

    fprintf(outfile, "\n\nAverage delay in queue%11.3f minutes\n\n",
        total_of_delays / num_custs_delayed);
    fprintf(outfile, "Average number in queue%10.3f\n\n",
            area_num_in_q / sim_time);
    fprintf(outfile, "Server utilization%15.3f\n\n",
            area_server_status / sim_time);
    fprintf(outfile, "(a) Average number in system  : %7.3f\n\n",
        area_num_in_sys / sim_time);
    fprintf(outfile, "(b) Average total time in system  : %7.3f minutes\n\n",
       total_t / cus_num_depart);
    fprintf(outfile, "(c) Max queue length  :   %d\n\n", max_queue);
    fprintf(outfile, "(d) Max delay in queue  : %7.3f minutes\n\n", max_delay);
    fprintf(outfile, "(e) Max time in the system  : %7.3f minutes\n\n", max_t);
    fprintf(outfile, "(f) Proportion of customers having a delay in queue in excess of 1 minute  : %7.3f\n\n", excess1 / num_custs_delayed);
    fprintf(outfile, "Time simulation ended%12.3f minutes", sim_time);
}


void update_time_avg_stats(void)  /* Update area accumulators for time-average
                                     statistics. */
{
    float time_since_last_event;

    /* Compute time since last event, and update last-event-time marker. */

    time_since_last_event = sim_time - time_last_event;
    time_last_event       = sim_time;

    /* Update area under nubmer-in-system function */

    area_num_in_sys += num_in_system * time_since_last_event;

    /* Update area under number-in-queue function. */

    area_num_in_q      += num_in_q * time_since_last_event;

    /* Update area under server-busy indicator function. */

    area_server_status += server_status * time_since_last_event;
}


float expon(float mean)  /* Exponential variate generation function. */
{
    /* Return an exponential random variate with mean "mean". */

    return -mean * log(lcgrand(1));
}

