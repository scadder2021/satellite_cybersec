/* ㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹ
ㅂ           2020-2021 CU BOULDER Spacecraft Cybersecurity          ㅂ
ㅂ                           Eloise Morris                          ㅂ
ㅂ                             내일을 향해!                            ㅂ
ㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹㄹ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>


/* sync bits for data word = 001b */
#define SYNC_BITS_DATA 0x1

/* sync bits for cmd word = 100b */
#define SYNC_BITS_CMD 0x4

/* sync bits for status word = 100b (which is same as cmd) */
#define SYNC_BITS_STATUS SYNC_BITS_CMD

/* sub address 5-bit max value */
#define SUB_ADDR_MAX 0x1F

/* Client class types */
#define BC_CLASS 1
#define RT_CLASS 2

/* Define the RT addresses of remote terminals and their spacecraft function (ie. the thruster
   is at RT address 1). More remote terminals may be added to improve fidelity to
   true satellite operation and their addresses and functions should be listed here. */
#define THRUSTER 0x01
#define MULTIPLEXOR 0x02
#define STARTRACKER 0x03
#define RADIO 0x04
#define REACTION_WHEEL 0x05
#define CAMERA 0x06

/* Defines ip address as broadcast address so all words are sent as broadcast */
#define IP_ADDR "10.0.0.255"
#define BUFFER_SIZE 1024 


/* ============== Spacecraft Command List ============ */

#define CAM_SNAP 'A'
#define FIRE_THRUSTER 'B'
#define CHANGE_SPEED_REACTION_WHEEL 'C'
#define CHECK_TELEMETRY 'D'
#define RADIO_TRANSMIT 'E'


#define TIMEOUT 10000000 //Sets bus controller timeout to specified time (in microseconds)

#ifndef BYTE_ORDER
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* =================== 1553 Word Structures =====================
   Here we define structures for the three types of words defined
   in the MIL-STD-1553 spec: command, status, and data words
   a structure "generic word" is also defined to take in a word of
   unknown variety until it can be assigned the appropriate structure
   by looking at sync bits. Previous iterations of the 1553 code
   sent two eight-bit characters as data words, which I have
   maintained. However, in the structure, the bits of these words
   are split in order to be consistant with byte lines */

/* Command word:
0                   1                   2                   
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Sync | RT Addr |T| Subaddr | wrd cnt |P| Pddng |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
typedef struct __attribute__((__packed__))
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char rt_address:5;

    unsigned char tr_bit:1;
    unsigned char subaddress:5;
    unsigned char word_count1:2;

    unsigned char word_count2:3;
    unsigned char parity_bit:1;
    unsigned char padding:4;
    #else
    unsigned char rt_address:5;
    unsigned char sync_bits:3;
    
    unsigned char word_count1:2;
    unsigned char subaddress:5;
    unsigned char tr_bit:1;

    unsigned char padding:4;
    unsigned char parity_bit:1;
    unsigned char word_count2:3;   
    #endif

}command_word_s;


/* Status word:
0                   1                   2                   
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Sync | RT Addr |E|I|S| Res |B|B|F|D|T|P| Pddng |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
typedef struct __attribute__((__packed__))
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char rt_address:5;

    unsigned char message_error:1;
    unsigned char instrumentation:1;
    unsigned char service_request:1;
    unsigned char reserved:3;
    unsigned char brdcst_received:1;
    unsigned char busy:1;

    unsigned char subsystem_flag:1;
    unsigned char dynamic_bus_control_accept:1;
    unsigned char terminal_flag:1;
    unsigned char parity_bit:1;
    unsigned char padding:4;
    #else
    unsigned char rt_address:5;
    unsigned char sync_bits:3;

    unsigned char busy:1;
    unsigned char brdcst_received:1;
    unsigned char reserved:3;
    unsigned char service_request:1;
    unsigned char instrumentation:1;
    unsigned char message_error:1;

    unsigned char padding:4;
    unsigned char parity_bit:1;
    unsigned char terminal_flag:1;
    unsigned char dynamic_bus_control_accept:1;
    unsigned char subsystem_flag:1;
    #endif

}status_word_s;

/* Data word:
0                   1                   2                   
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Sync |   CHAR 1    |      CHAR 2     |P| Pddng |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

typedef struct __attribute__((__packed__)) 
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char character_A1:5;

    unsigned char character_A2:3;
    unsigned char character_B1:5;

    unsigned char character_B2:3;
    unsigned char parity_bit:1;
    unsigned char padding:4;
    #else
    unsigned char character_A1:5;
    unsigned char sync_bits:3;

    unsigned char character_B1:5;
    unsigned char character_A2:3;

    unsigned char padding:4;
    unsigned char parity_bit:1;
    unsigned char character_B2:3;
    #endif

}data_word_s;

typedef struct __attribute__((__packed__))
{
    #if BYTE_ORDER == BIG_ENDIAN
    unsigned char sync_bits:3;
    unsigned char reserved0:5;

    unsigned char reserved1:8;
 
    unsigned char reserved2:8;
    unsigned char padding:4;
    #else
    unsigned char reserved0:5;
    unsigned char sync_bits:3;
    
    unsigned char reserved1:8;

    unsigned char padding:4;
    unsigned char reserved2:4;
    #endif
}generic_word_s;


/* creates a structure to store unique rt/bc data passed by the
   simulator codes */
typedef struct
{
    unsigned int source_port;
    unsigned int destination_port;
    unsigned int rt_address;
    unsigned int user_class; //specifies whether a user is a bus controller or remote terminal
}client_cb;


/* MACROs to split a data word character into its 2 corresponding
  fields in data_word_s and recombine them. This function is needed
  since we must assign bits in the word in relation to byte
  boundaries for it to work correctly.  */
#define SPLIT_CHAR(in_char, out_char_upper5, out_char_lower3) \
{                                                             \
    out_char_upper5 = ((int)in_char>>3) & 0x1F;               \
    out_char_lower3 = ((int)in_char & 0x7);                   \
}

#define COMBINE_CHAR(in_char_upper5, in_char_lower3)          \
    (in_char_upper5<<3) | (in_char_lower3)               


/* Represents the RTs memory and is used to send data to the BC.
   This could be edited in future to create a more realistic
   representation of RT memory */
char rt_memory_2d[64][2] = { {'A','A'},
                             {'A','B'},
                             {'A','C'},
                             {'A','D'},
                             {'A','E'},
                             {'A','F'},
                             {'A','H'},
                             {'A','I'},
                             {'A','J'},
                             {'A','K'},
                             {'A','L'},
                             {'A','M'},
                             {'A','N'},
                             {'A','O'},
                             {'A','P'},
                             {'A','Q'},
                             {'A','R'},
                             {'A','S'},
                             {'A','T'},
                             {'A','U'},
                             {'A','V'},
                             {'A','W'},
                             {'A','X'},
                             {'A','Y'},
                             {'A','Z'},
                             {'B','A'},
                             {'B','B'},
                             {'B','C'},
                             {'B','D'},
                             {'B','E'},
                             {'B','F'},
                             {'B','G'},
                             {'B','H'},
                             {'B','I'},
                             {'B','J'},
                             {'B','K'},
                             {'B','L'},
                             {'B','M'},
                             {'B','N'},
                             {'B','O'},
                             {'B','P'},
                             {'B','Q'},
                             {'B','R'},
                             {'B','S'},
                             {'B','T'},
                             {'B','U'},
                             {'B','V'},
                             {'B','W'},
                             {'B','X'},
                             {'B','Y'},
                             {'B','Z'},
                             {'C','A'},
                             {'C','B'},
                             {'C','C'},
                             {'C','D'},
                             {'C','E'},
                             {'C','F'},
                             {'C','G'},
                             {'C','H'},
                             {'C','I'},
                             {'C','J'},
                             {'C','K'},
                             {'C','L'},                            
                             {'C','M'} };

/*============================ Global Variables ===============================*/

int num_pending_words; //variable to allow bus controller to wait for all requested data

int waiting_sc_command = 0; //variable to wait for spacecraft command



generic_word_s queue[256]; //create a buffer to store 1553 data to be written to shared memory TODO

client_cb control_block; //global variable for client control block structure

/* ==================== MODE CODES (FOR REFERENCE) ========================

   0x00 Dynamic Bus Control
   0x01 Synchronize
   0x02 Transmit Status Word
   0x03 Initiate Self Test
   0x04 Transmitter Shutdown
   0x05 Override Transmitter Shutdown
   0x06 Inhibit Terminal Flag
   0x07 Override Inhibit Terminal Flag
   0x08 Reset Remote Terminal
   0x09-0x0F Reserved
   0x10 Transmit Vector Word
   0x11 Synchronize (w/data)
   0x12 Transmit Last Command Word
   0x13 Transmit BIT word
   0x14 Selected transmitter shutdown
   0x15 Override transmitter shutdown
   0x16-0x1F Reserved

   ======================================================================== */

typedef enum
{
   DYNAMIC_BUS_CONTROL,
   SYNCHRONIZE,
   TRANSMIT_STATUS_WORD,
   INTIATE_SELF_TEST,
   TRANSMITTER_SHUTDOWN,
   OVERRIDE_TRANSMITTER_SHUTDOWN,
   INHIBIT_TERMINAL_FLAG,
   OVERRIDE_INHIBIT_TERMINAL_FLAG,
   RESET_REMOTE_TERMINAL,
   TRANSMIT_VECTOR_WORD = 0x10, //Defined to skip over codes reserved in standard
   SYNCHRONIZE_DATA,
   TRANSMIT_LAST_COMMAND_WORD,
   TRANSMIT_BIT_WORD,
   SELECTED_TRANSMITTER_SHUTDOWN,
   OVERRIDE_SELECTED_TRANSMITTER_SHUTDOWN  
} mode_code_e;



/* ============= FOREWARD DECLARATIONS ============= */

void analyze_command_word(command_word_s * command_word);
void decode_data_word(data_word_s * data_word);
void build_bc_data_word(char message_byte1, char message_byte2); 
void build_command_word(int rt_address, char tr_bit, int subaddress, int word_count);
void analyze_status_word(status_word_s * status_word);
void interpret_incoming_frame_bc(generic_word_s * generic_word);
void interpret_incoming_frame_rt(generic_word_s * generic_word);
void send_data(generic_word_s *data); 
void analyze_mode_code(command_word_s * command_word);
void interpret_sc_command(data_word_s * data_word);
void calculate_parity_bit(command_word_s *data);


// A hacky way to print command word bits (testing)
void print_word(command_word_s * word_check)
{
    #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
    #define BYTE_TO_BINARY(word_check)  \
        (word_check->sync_bits & 0x04 ? '1' : '0'), \
        (word_check->sync_bits & 0x02 ? '1' : '0'), \
        (word_check->sync_bits & 0x01 ? '1' : '0'), \
        (word_check->rt_address & 0x10 ? '1' : '0'), \
        (word_check->rt_address & 0x08 ? '1' : '0'), \
        (word_check->rt_address & 0x04 ? '1' : '0'), \
        (word_check->rt_address & 0x02 ? '1' : '0'), \
        (word_check->rt_address & 0x01 ? '1' : '0'), \
        (word_check->tr_bit & 0x01 ? '1' : '0'), \
        (word_check->subaddress & 0x10 ? '1' : '0'), \
        (word_check->subaddress & 0x08 ? '1' : '0'), \
        (word_check->subaddress & 0x04 ? '1' : '0'), \
        (word_check->subaddress & 0x02 ? '1' : '0'), \
        (word_check->subaddress & 0x01 ? '1' : '0'), \
        (word_check->word_count1 & 0x02 ? '1' : '0'), \
        (word_check->word_count1 & 0x01 ? '1' : '0'), \
        (word_check->word_count2 & 0x04 ? '1' : '0'), \
        (word_check->word_count2 & 0x02 ? '1' : '0'), \
        (word_check->word_count2 & 0x01 ? '1' : '0'), \
        (word_check->parity_bit & 0x01 ? '1' : '0')

        printf("Command word: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word_check));

}

//A hacky way to print data word bits (for testing)
void print_data_word(data_word_s * word_check)
{
    #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
    #define BYTE_TO_BINARY2(word_check)  \
        (word_check->sync_bits & 0x04 ? '1' : '0'), \
        (word_check->sync_bits & 0x02 ? '1' : '0'), \
        (word_check->sync_bits & 0x01 ? '1' : '0'), \
        (word_check->character_A1 & 0x10 ? '1' : '0'), \
        (word_check->character_A1 & 0x08 ? '1' : '0'), \
        (word_check->character_A1 & 0x04 ? '1' : '0'), \
        (word_check->character_A1 & 0x02 ? '1' : '0'), \
        (word_check->character_A1 & 0x01 ? '1' : '0'), \
        (word_check->character_A2 & 0x04 ? '1' : '0'), \
        (word_check->character_A2 & 0x02 ? '1' : '0'), \
        (word_check->character_A2 & 0x01 ? '1' : '0'), \
        (word_check->character_B1 & 0x10 ? '1' : '0'), \
        (word_check->character_B1 & 0x08 ? '1' : '0'), \
        (word_check->character_B1 & 0x04 ? '1' : '0'), \
        (word_check->character_B1 & 0x02 ? '1' : '0'), \
        (word_check->character_B1 & 0x01 ? '1' : '0'), \
        (word_check->character_B2 & 0x04 ? '1' : '0'), \
        (word_check->character_B2 & 0x02 ? '1' : '0'), \
        (word_check->character_B2 & 0x01 ? '1' : '0'), \
        (word_check->parity_bit & 0x01 ? '1' : '0')

        printf("Data word: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY2(word_check));
        //printf("Characters: %c%c\n", (word_check->character_A1<<3) | (word_check->character_A2), (word_check->character_B1<<3) | (word_check->character_B2));
        printf("Characters: %c%c\n", 
                COMBINE_CHAR(word_check->character_A1, word_check->character_A2),
                COMBINE_CHAR(word_check->character_B1, word_check->character_B2));

}

// A hacky way to print 3 bytes from any pointer (for testing)
void print_void(void * void_ptr)
{
    char * word_check = (char *)void_ptr;

    #define BYTE_TO_BINARY_PATTERN3 "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
    #define BYTE_TO_BINARY3(word_check)  \
        (word_check[0] & 0x80 ? '1' : '0'), \
        (word_check[0] & 0x40 ? '1' : '0'), \
        (word_check[0] & 0x20 ? '1' : '0'), \
        (word_check[0] & 0x10 ? '1' : '0'), \
        (word_check[0] & 0x08 ? '1' : '0'), \
        (word_check[0] & 0x04 ? '1' : '0'), \
        (word_check[0] & 0x02 ? '1' : '0'), \
        (word_check[0] & 0x01 ? '1' : '0'), \
        (word_check[1] & 0x80 ? '1' : '0'), \
        (word_check[1] & 0x40 ? '1' : '0'), \
        (word_check[1] & 0x20 ? '1' : '0'), \
        (word_check[1] & 0x10 ? '1' : '0'), \
        (word_check[1] & 0x08 ? '1' : '0'), \
        (word_check[1] & 0x04 ? '1' : '0'), \
        (word_check[1] & 0x02 ? '1' : '0'), \
        (word_check[1] & 0x01 ? '1' : '0'), \
        (word_check[2] & 0x80 ? '1' : '0'), \
        (word_check[2] & 0x40 ? '1' : '0'), \
        (word_check[2] & 0x20 ? '1' : '0'), \
        (word_check[2] & 0x10 ? '1' : '0'), \
        (word_check[2] & 0x08 ? '1' : '0'), \
        (word_check[2] & 0x04 ? '1' : '0'), \
        (word_check[2] & 0x02 ? '1' : '0'), \
        (word_check[2] & 0x01 ? '1' : '0')
        printf("Void word:    "BYTE_TO_BINARY_PATTERN3, BYTE_TO_BINARY3(word_check));

}

/* ========================================================================
   The following functions are used to create the command/data words sent 
   by the BC to the RT
   ====================================================================== */


/* This function sends a command word for the RT to receive data and 
   then breaks the message into segments to be sent to the RT as 
   data words */
void send_data_to_rt(int rt_address, int subaddress, char message[])
{
    int message_length = strlen(message); //gets length of message so we can determine how many words to send
    int number_data_words;
    int data_word_count = 0; //used to increment number of words built
    int character_number = 0; //used to increment through characters in the message

    if((message_length & 0x1) == 0) //check if even
    {
        number_data_words = message_length/2; //we send 2 8 bit characters per word
    }
    else
    {
        number_data_words = (message_length / 2) + 1; //since we can only send 2 8bit characters per word, if there is one left over it is sent in own word
    }
    //printf("Number of data words: %d\n", number_data_words);
    build_command_word(rt_address, 'R', subaddress, number_data_words); //first send command word instructing RT to receive

    for(data_word_count = 0; data_word_count < number_data_words; data_word_count++) //construct as many data words as needed to send entire message
    {
        if(character_number + 1 > message_length)
        {
            build_bc_data_word(message[character_number], ' '); 
        }
        else
        {
            build_bc_data_word(message[character_number], message[character_number+1]); 
        }
        character_number += 2;

    }   

}

/* This fuction sends a command word for the RT to send data*/

void request_data_from_rt(int rt_address, int subaddress, int word_count)
{
    build_command_word(rt_address, 'T', subaddress, word_count);
    num_pending_words = word_count + 1; //set the incrementer so it can wait until all requested data is received. The plus 1 accounts for the status word.
    int wait_time = 0;
    while (num_pending_words > 0)
    {
        if(wait_time >= TIMEOUT && num_pending_words == (word_count + 1))
        {
            printf("The timeout has been reached without receiving any status or data words.\n");
            break;
        }
        else
        {
            usleep(10);
            wait_time+=10;
        }
    }

}

void send_sc_command(int rt_address, char command)
{
    build_command_word(rt_address, 'R', 0, 1);
    char sc_command_indicator = '/';
    build_bc_data_word(sc_command_indicator, command);
}

/* This fuction builds command words according the 1553 spec */
void send_mode_code(int rt_address, int mode_code)
{
    command_word_s mode_code_command;
    mode_code_command.sync_bits = SYNC_BITS_CMD;
    mode_code_command.rt_address = rt_address;
    if (mode_code == 0x11 || mode_code == 0x14 || mode_code == 0x15)
    {
        mode_code_command.tr_bit = 0;
    }
    else
    {
        mode_code_command.tr_bit = 0;
    }
    mode_code_command.subaddress = 0; //For mode codes, the subaddress is 0 or 31
    mode_code_command.word_count1 = (mode_code >> 3) & 0x3;
    mode_code_command.word_count2 = (mode_code) & 0x7;
    mode_code_command.padding = 0;
    calculate_parity_bit(&mode_code_command);
    send_data((generic_word_s*)&mode_code_command);

}

void build_command_word(int rt_address, char tr_bit, int subaddress, int word_count)
{
    command_word_s command_word;

    command_word.sync_bits = SYNC_BITS_CMD; 
    command_word.rt_address = rt_address;
    if (tr_bit == 'T')
        command_word.tr_bit = 1;
    else if (tr_bit == 'R')
    {
        command_word.tr_bit = 0;
    }
    else
        printf("Invalid T/R bit.\n");
    command_word.subaddress = subaddress;
    command_word.word_count1 = (word_count >> 3) & 0x3; //TODO
    command_word.word_count2 = (word_count) & 0x7;
    command_word.padding = 0;
    calculate_parity_bit(&command_word);
    //print_word(&command_word);
    send_data((generic_word_s*)&command_word);
}

/* This fuction builds data words according the 1553 spec */

void build_bc_data_word(char message_byte1, char message_byte2)
{
    data_word_s data_word;
    data_word.sync_bits = SYNC_BITS_DATA;
    SPLIT_CHAR(message_byte1, data_word.character_A1, data_word.character_A2);

    SPLIT_CHAR(message_byte2, data_word.character_B1, data_word.character_B2);
    data_word.padding = 0;
    calculate_parity_bit((command_word_s *)&data_word);
    //print_data_word(&data_word);
    send_data((generic_word_s*)&data_word); 
}

/* =======================================================================
   The following functions are used to create the status/data words sent
   by the RTs based on the command words received
   ======================================================================= */

void build_rt_data_word(int subaddress, int data_word_count)
{
    data_word_s data_word;
    int data_words_sent = 0;

    /* input validation */
    if(subaddress > SUB_ADDR_MAX) //TODO Sub-address should start anywhere but not buffer overflow
    {
        printf("Invalid subaddress (build_rt_data_word)");
        return;
    }

    for(data_words_sent = 0; data_words_sent < data_word_count; data_words_sent++)
    {
        data_word.sync_bits = SYNC_BITS_DATA;

        SPLIT_CHAR((int)rt_memory_2d[subaddress+data_words_sent][0], data_word.character_A1, data_word.character_A2);
        SPLIT_CHAR((int)rt_memory_2d[subaddress+data_words_sent][1], data_word.character_B1, data_word.character_B2);
        data_word.padding = 0;
        calculate_parity_bit((command_word_s *)&data_word);
        
        send_data((generic_word_s*)&data_word);
    }
}

/* Status word values have been hard coded to the specific values
   used in previous iterations of 1553 code. This could be
   updated for greater fidelity to actual operation */
void build_status_word()
{
    status_word_s status_word;
    status_word.sync_bits = SYNC_BITS_STATUS;
    status_word.rt_address = control_block.rt_address;
    status_word.message_error = 0;
    status_word.instrumentation = 0;
    status_word.service_request = 0;
    status_word.reserved = 0;
    status_word.brdcst_received = 0;
    status_word.busy = 0;
    status_word.subsystem_flag = 0;
    status_word.dynamic_bus_control_accept = 0;
    status_word.terminal_flag = 1;
    status_word.padding = 0;
    calculate_parity_bit((command_word_s *)&status_word);

    send_data((generic_word_s*)&status_word);

}

/* This function calculates and set the parity bit of
   words for odd parity. It is important that the padding
   of each word be set to zero as well, so it does not 
   affect the parity of the 1553 data */
void calculate_parity_bit(command_word_s *data)
{
    unsigned int x = *((unsigned int *)data);
    x = x ^ (x >> 1);
    x = x ^ (x >> 2);
    x = x ^ (x >> 4);
    x = x ^ (x >> 8);
    x = x ^ (x >> 16);
 
    if (x & 1)
    {
        data->parity_bit = 0;
    }
    else
    {
        data->parity_bit = 1;
    }
    
}


/* ============================================================
   The following functions are used to interpret incoming
   words and process them accordingly
   ============================================================ */

/* This function determines whether an incoming word received by an
   RT is a data word or command word */
void interpret_incoming_frame_rt(generic_word_s * generic_word)
{
    if(generic_word->sync_bits == SYNC_BITS_CMD)
    {
        analyze_command_word((command_word_s *)generic_word);
    }
    else if(generic_word->sync_bits == SYNC_BITS_DATA)
    {
        //printf("data word received\n"); //testing
        //print_void(generic_word);
        //print_data_word(generic_word);
        decode_data_word((data_word_s *)generic_word);
    }
    else
    {
        printf("Invalid word received from BC.");
        return;
    }
    
    
}

/* This function determines whether an incoming word received by an
   BC is a data word or command word */
void interpret_incoming_frame_bc(generic_word_s * generic_word)
{
    if(generic_word->sync_bits == SYNC_BITS_STATUS)
    {
        if(num_pending_words > 0)
        {
            num_pending_words -= 1;
        }
        analyze_status_word((status_word_s *)generic_word);
    }
    else if(generic_word->sync_bits == SYNC_BITS_DATA)
    {
        num_pending_words -= 1;
        decode_data_word((data_word_s *)generic_word);
    }
    else
    {
        printf("Invalid word received from RT.");
        return;
    }
}

/* This function determines if a command word is instructing to
   transmit or receive and responds accordingly */
void analyze_command_word(command_word_s * command_word)
{
    if (command_word->rt_address == control_block.rt_address)
    {
        //print_word(command_word);
        if(command_word->tr_bit == 0)
        {
            if (command_word->subaddress == 0 || command_word->subaddress == 31) 
            {
                build_status_word();
                analyze_mode_code(command_word);
            }
        }
        else if(command_word->tr_bit == 1)
        {
            if (command_word->subaddress == 0 || command_word->subaddress == 31) //TODO
            {
                build_status_word();
                analyze_mode_code(command_word);
            }
            else
            {
                build_status_word();
                build_rt_data_word(command_word->subaddress, ((command_word->word_count1<<3) | command_word->word_count2)); //TODO
            }
        
        }
    }
    
}

void analyze_mode_code(command_word_s * command_word)
{
    if (command_word->subaddress == 0)
    {
        waiting_sc_command = 1;
    }
    else
    {
        mode_code_e mode_code = (command_word->word_count1<<3) | (command_word->word_count2); //TODO
        switch(mode_code)
        {
            case DYNAMIC_BUS_CONTROL:
                printf("Initializing mode code: dynamic bus control\n");
                break;
            case SYNCHRONIZE:
                printf("Initializing mode code: synchronize\n");
                break;
            case TRANSMIT_STATUS_WORD:
                printf("Initializing mode code: synchronize\n");
                break;
            case INTIATE_SELF_TEST:
                printf("Initializing mode code: self test\n");
                break;
            case TRANSMITTER_SHUTDOWN:
                printf("Initializing mode code: transmitter shutdown\n");
                break;
            case OVERRIDE_TRANSMITTER_SHUTDOWN:
                printf("Initializing mode code: override transmitter shutdown\n");
                break;
            case INHIBIT_TERMINAL_FLAG:
                printf("Initializing mode code: INHIBIT_TERMINAL_FLAG\n");
                break;
            case OVERRIDE_INHIBIT_TERMINAL_FLAG:
                printf("Initializing mode code: OVERRIDE_INHIBIT_TERMINAL_FLAG\n");
                break;
            case RESET_REMOTE_TERMINAL:
                printf("Initializing mode code: RESET_REMOTE_TERMINAL\n");
                break;
            case TRANSMIT_VECTOR_WORD:
                printf("Initializing mode code: TRANSMIT_VECTOR_WORD\n");
                break;
            case SYNCHRONIZE_DATA:
                printf("Initializing mode code: SYNCHRONIZE_DATA\n");
                break;
            case TRANSMIT_LAST_COMMAND_WORD:
                printf("Initializing mode code: TRANSMIT_LAST_COMMAND_WORD\n");
                break;
            case TRANSMIT_BIT_WORD:
                printf("Initializing mode code: TRANSMIT_BIT_WORD\n");
                break;
            case SELECTED_TRANSMITTER_SHUTDOWN:
                printf("Initializing mode code: SELECTED_TRANSMITTER_SHUTDOWN\n");
                break;
            case OVERRIDE_SELECTED_TRANSMITTER_SHUTDOWN:
                printf("Initializing mode code: OVERRIDE_TRANSMITTER_SHUTDOWN\n");
                break;
        }

    }
}

/* Prints data words received */
void decode_data_word(data_word_s * data_word)
{
    //printf("decoding data word\n"); testing
    if((COMBINE_CHAR(data_word->character_A1, data_word->character_A2)) == '/')
    {
        if(waiting_sc_command == 1)
        {
            interpret_sc_command(data_word);
            waiting_sc_command = 0;
        }
    }
    else
    {
        printf("%c%c\n", 
            COMBINE_CHAR(data_word->character_A1, data_word->character_A2), 
            COMBINE_CHAR(data_word->character_B1, data_word->character_B2));
    }
}

void interpret_sc_command(data_word_s * data_word)
{
    //printf("interpreting sc command\n"); 
    if((COMBINE_CHAR(data_word->character_B1, data_word->character_B2)) == 'A') //testing
    {
        printf("Taking picture\n");
    }
    else if((COMBINE_CHAR(data_word->character_B1, data_word->character_B2)) == 'B')
    {
        printf("Firing thruster\n");
    }
    else if((COMBINE_CHAR(data_word->character_B1, data_word->character_B2)) == 'C')
    {
        printf("Adjusting reaction wheel speed\n");
    }
    else if((COMBINE_CHAR(data_word->character_B1, data_word->character_B2)) == 'D')
    {
        printf("Checking telemetry values\n");
    }
    else
    {
        printf("Transmitting data to ground station\n");
    }
}

/* Prints status words */
void analyze_status_word(status_word_s * status_word)
{

    printf("Status word: terminal_flag_bit: %d, busy bit: %d, brdcst_recvd_bit: %d, rt_address: %d, message_error_bit: %d, subsystem_flag_bit: %d,\
     dynamic_bus_control_accpt: %d, reserved_bits: %d, service_request_bit: %d, instrumentation_bit: %d\n", status_word->terminal_flag, status_word->busy,\
     status_word->brdcst_received, status_word->rt_address, status_word->message_error, status_word->subsystem_flag, status_word->dynamic_bus_control_accept,\
     status_word->reserved, status_word->service_request, status_word->instrumentation);

}


/* ===========================================================
   The following functions are used to initialize 
   the simulators and create a UDP broadcast socket 
   =========================================================== */

//starts a socket to listen for incoming packets
void *initialize_listener() 
{ 
    printf("Initializing listener\n");
    int socket_listener; 
    char buffer[BUFFER_SIZE];  
    struct sockaddr_in address; 
    int n;
    unsigned int len;

    int so_broadcast = 1;

    if ( (socket_listener = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    { 
        perror("Socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    }

    if(setsockopt(socket_listener, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast)) < 0)
    {
        perror("Error in setting broadcast option\n");
        exit(EXIT_FAILURE);
    }


    memset(&address, 0, sizeof(address)); 
       
    address.sin_family = AF_INET; 
    address.sin_port = htons(control_block.source_port); //Sets port to the source port passed by the RT simulator and set in the control block
    address.sin_addr.s_addr = INADDR_ANY; //Binds to all interfaces
      
    if ( bind(socket_listener, (const struct sockaddr *)&address, sizeof(address)) < 0 ) 
    { 
        perror("Bind failed\n"); 
        exit(EXIT_FAILURE); 
    }   

    while(1)
    {
          
        n = recvfrom(socket_listener, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *) &address, &len); 
        buffer[n] = '\0';
        printf("packet received\n"); //testing
        print_void(buffer); //testing
        if (control_block.user_class == BC_CLASS) //in order to know how to interpret the incoming word, the user class is referenced (which is set during initialization)
        {
            interpret_incoming_frame_bc((generic_word_s*)buffer);
        }
        else if (control_block.user_class == RT_CLASS)
        {
            interpret_incoming_frame_rt((generic_word_s*)buffer);
        }
        else
        {
            printf("Invalid user class: %d\n", control_block.user_class);
        }
    }
    return NULL;
    
} 


/* creates a socket to send data */
void send_data(generic_word_s *data) 
{
    int socket_sender; 
    char buffer[BUFFER_SIZE];
    int so_broadcast;
    so_broadcast = 1; 
    struct sockaddr_in address;

    if ( (socket_sender = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    { 
        perror("socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    }

    if(setsockopt(socket_sender, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast)) < 0)
    {
        perror("Error in setting broadcast option\n");
        exit(EXIT_FAILURE);
    }


    memset(&address, 0, sizeof(address)); 
      
    address.sin_family = AF_INET;  
    address.sin_port = htons(control_block.destination_port); 
    address.sin_addr.s_addr = inet_addr(IP_ADDR); 

    int n, len;
    //print_void((void *)data); DEBUG
    sendto(socket_sender, (const void *)data, sizeof(generic_word_s), 0, (const struct sockaddr *) &address, sizeof(address));
    
}

/* This function initializes the simulators by creating a control block
   containing all the necessary information and starting a listening
   socket */
void initialize_library(int source_port, int destination_port, int rt_address, int class)
{
    
    control_block.source_port = source_port;
    control_block.destination_port = destination_port;
    control_block.rt_address = rt_address;
    control_block.user_class = class;

    pthread_t thread1;
    int listener;
    listener = pthread_create(&thread1, NULL, initialize_listener, NULL);

}

/*

FUTURE WORK:

Spacecraft commands should be improved
Create 1553 PCAP parser
orbits? (LEO ~99 min/orbit)
Change commands to hex representations

*/
