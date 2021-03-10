#include "BC_Simulator.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <unistd.h>
#include <time.h>


#define PERCENT_STATION_COMMANDS 5 //set the percentage of random data to send


/* Generate a random number between [min,max] (inclusive) */
#define RANDOM(min, max) \
  (min + (rand() % (max - min + 1)))

/* =======================================================
   This function serves to add a degree of randomness to
   our 1553 bus data. Since ground stations can send
   mission-focused commands to satellites as necessary,
   this traffic represents these less predictable commands.
   ====================================================== */
void generate_random_data(int rt_address)
{
    int word_type;
    word_type = rand() % 2; //chooses randomly whether to send data, request data, or send a mode code

    if(word_type == 0)
    {
        send_data_to_rt(rt_address, 0, "Some Message"); //TODO future: allow for dynamic selection of subaddress and message
    }
    else if (word_type == 1)
    {
        request_data_from_rt(rt_address, RANDOM(1, 30), RANDOM(1, 62));
    }
}

/* =======================================================
   The following functions define spacecraft commands and
   send specific actions to the RTs. For instance, if we 
   want to point the spacecraft toward the ground station,
   we can send attitude adjustment commands to thrusters
   and reaction wheels. This is an area where additional
   development is needed to improve the realisticness of
   the simulation.
   ====================================================== */
void point_sc_at_groundstation()
{
    printf("pointing at ground");
    send_sc_command(REACTION_WHEEL, CHANGE_SPEED_REACTION_WHEEL);
    send_sc_command(THRUSTER, FIRE_THRUSTER);
}

void point_sc_at_target()
{
    printf("pointing at target");
    send_sc_command(REACTION_WHEEL, CHANGE_SPEED_REACTION_WHEEL); //ADD COORDINATES OF TARGET?
    send_sc_command(THRUSTER, FIRE_THRUSTER);
}

void send_data_to_ground()
{
    printf("sending data to ground");
    send_sc_command(RADIO, RADIO_TRANSMIT);
}

void take_picture()
{
    printf("taking pic");
    send_sc_command(CAMERA, CAM_SNAP);
}

void check_telemetry()
{
    printf("checking telem");
    request_data_from_rt(MULTIPLEXOR, RANDOM(1, 15), RANDOM(1, 32));
}

void check_status_sc_systems()
{
    printf("check status");
    request_data_from_rt(STARTRACKER, RANDOM(1, 15), RANDOM(1, 32));
    request_data_from_rt(RADIO, RANDOM(1, 15), RANDOM(1, 32));
}

/* =======================================================
   The main function defines when and what commands the
   satellite receives from the flight software. Currently,
   it points at the ground station and sends data when
   passing over station, points at target and take picture
   when over target, and periodically checks telemetry and
   sc system data at all other times.
   ====================================================== */
int main()
{
    init_bc();
    sleep(1);
    //clock_t time_at_start = clock();
    //clock_t time_over_target = clock();
    while (1)
    {
        /* Since we are not modeling satellite orbits in this code,
           time elapsed is used as a proxy for a completed orbit.
           LEO satellites take approx. 90 minutes to orbit the earth
           and so are over the ground station every 90 minutes. This
           has been reduced to 9 minutes in this simulation. */
        /*
        clock_t current_time = clock();
        int send_ground_station_command = RANDOM(1, 100);

        if(current_time - time_at_start == 9)
        {
            point_sc_at_groundstation();
            send_data_to_ground();
            time_at_start = clock();
        }
        else if(current_time - time_at_start == 5)
        {
            point_sc_at_target();
            take_picture();
        }
        else if (send_ground_station_command <= PERCENT_STATION_COMMANDS)
        {
            generate_random_data((rand() % 6) + 1); 
        }
        else
        {
        */
            //mode_code_e mode_code1 = RANDOM(0, 8);
            //mode_code_e mode_code2 = RANDOM(10, 15);
            
            check_telemetry();
            sleep(10);
            check_status_sc_systems();
            sleep(10);
            point_sc_at_groundstation();
            send_data_to_ground();
            sleep(10);
            point_sc_at_target();
            take_picture();
            sleep(10);
           

 
    }

    return 0;
}
