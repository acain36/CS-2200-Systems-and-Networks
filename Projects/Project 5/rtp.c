#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"
#include "network.h"
#include "rtp.h"

/**
 * PLESE ENTER YOUR INFORMATION BELOW TO RECEIVE MANUAL GRADING CREDITS
 * Name: YOUR NAME
 * GTID: YOUR GTID
 * Fall 2022
 */

typedef struct message {
    char *buffer;
    int length;
} message_t;

/* ================================================================ */
/*                  H E L P E R    F U N C T I O N S                */
/* ================================================================ */

/**
 * --------------------------------- PROBLEM 1 --------------------------------------
 * 
 * Convert the given buffer into an array of PACKETs and returns the array.  The 
 * value of (*count) should be updated so that it contains the number of packets in 
 * the array
 * 
 * @param buffer pointer to message buffer to be broken up packets
 * @param length length of the message buffer.
 * @param count number of packets in the returning array
 * 
 * @returns array of packets
 */
packet_t *packetize(char *buffer, int length, int *count) {

    int num_packets = length/MAX_PAYLOAD_LENGTH;
    int partial_packet = length % MAX_PAYLOAD_LENGTH;

    if(partial_packet != 0) {   //This means there is a partial packet needed for rest of message
        num_packets++;
    }

    int space = sizeof(packet_t) * (unsigned int)(num_packets);
    
    //allocate memory space
    packet_t *packets = malloc(space);
    //set memory space to have 0 at all locations
    memset(packets, 0, space);

    for(int i = 0; i < num_packets; i++) {
        if(i < (num_packets - 1)){      //this packet is not the last in message
            packets[i].type = DATA;
            packets[i].payload_length = MAX_PAYLOAD_LENGTH;
        } else{                         //this packet is the last packet in the message
            packets[i].type = LAST_DATA;
            packets[i].payload_length = length % MAX_PAYLOAD_LENGTH;
        }

        for(int j = 0; j < packets[i].payload_length; j++) {
            packets[i].payload[j] = buffer[(i * MAX_PAYLOAD_LENGTH) + j];
        }

        packets[i].checksum = checksum(packets[i].payload, packets[i].payload_length);
    }

    *count = num_packets;

    return packets;
    
    
}

/**
 * --------------------------------- PROBLEM 2 --------------------------------------
 * 
 * Compute a checksum based on the data in the buffer.
  * 
 * Checksum calcuation: For the first two ASCII characters of the buffer, 
 * multiply each of their values by three and add them together. Then for the next 
 * two characters, divide each of their values by three and add them to the total. 
 * Repeat this for each group of two characters until the buffer is empty.
 * 
 * Example: "abcd" checksum = (a * 3) + (b * 3) + (c / 3) + (d / 3)
 * 
 * @param buffer pointer to the char buffer that the checksum is calculated from
 * @param length length of the buffer
 * 
 * @returns calcuated checksum
 */
int checksum(char *buffer, int length) {

    int even = 1;
    int sum = 0;

    for(int i = 0; i < length; i++) {
        if (even == 1){
            sum += (buffer[i] * 3);
            if(i++ < length) {
                sum += (buffer[i] * 3);
            }
            even = 0;
        } else {
            sum += (int)(buffer[i] / 3);
            if(i++ < length) {
                sum += (int)(buffer[i] / 3);
            }
            even = 1;
        }
    }
    return sum;
}


/* ================================================================ */
/*                      R T P       T H R E A D S                   */
/* ================================================================ */

static void *rtp_recv_thread(void *void_ptr) {

    rtp_connection_t *connection = (rtp_connection_t *) void_ptr;

    do {
        message_t *message;
        int buffer_length = 0;
        char *buffer = NULL;
        packet_t packet;

        /* Put messages in buffer until the last packet is received  */
        do {
            if (net_recv_packet(connection->net_connection_handle, &packet) <= 0 || packet.type == TERM) {
                /* remote side has disconnected */
                connection->alive = 0;
                pthread_cond_signal(&connection->recv_cond);
                pthread_cond_signal(&connection->send_cond);
                break;
            }

            /*  ----  FIXME  ----
            *
            * 1. check to make sure payload of packet is correct
            * 2. send an ACK or a NACK, whichever is appropriate
            * 3. if this is the last packet in a sequence of packets
            *    and the payload was corrupted, make sure the loop
            *    does not terminate
            * 4. if the payload matches, add the payload to the buffer
            */
            if(packet.type == DATA || packet.type == LAST_DATA) {
                packet_t * return_info = malloc(sizeof(packet_t));

                return_info -> type = NACK;

                if(packet.checksum == checksum(packet.payload, packet.payload_length)) {
                    return_info -> type = ACK;

                    buffer = realloc(buffer, (size_t)(buffer_length + packet.payload_length));

                    for(int i = 0; i < packet.payload_length; i++) {
                        buffer[buffer_length + i] = packet.payload[i];
                    }

                    buffer_length += packet.payload_length;
                } else {    //in this circumstance, NACK is being sent already as type
                    if(packet.type == LAST_DATA) {
                        packet.type = DATA;
                    }
                }

                net_send_packet(connection->net_connection_handle, return_info);
                free(return_info);
            } 


            /*
            *  What if the packet received is not a data packet?
            *  If it is a NACK or an ACK, the sending thread should
            *  be notified so that it can finish sending the message.
            *   
            *  1. add the necessary fields to the CONNECTION data structure
            *     in rtp.h so that the sending thread has a way to determine
            *     whether a NACK or an ACK was received
            *  2. signal the sending thread that an ACK or a NACK has been
            *     received.
            */
            if(packet.type == ACK || packet.type == NACK) {

                pthread_mutex_lock(&connection->ack_mutex);

                if(packet.type == ACK) {
                    connection->acknowledged = 2;
                } else if(packet.type == NACK) {
                    connection->acknowledged = 1;
                }

                pthread_cond_signal(&connection->ack_cond);
                pthread_mutex_unlock(&connection->ack_mutex);
            }

        } while (packet.type != LAST_DATA);

        if (connection->alive == 1) {
            /*  ----  FIXME: Part II-C ----
            *
            * Now that an entire message has been received, we need to
            * add it to the queue to provide to the rtp client.
            *
            * 1. Add message to the received queue.
            * 2. Signal the client thread that a message has been received.
            */

           message = (message_t *)(malloc(sizeof(message_t)));

           message->buffer = buffer;
           message->length = buffer_length;

           pthread_mutex_lock(&connection->recv_mutex);

           queue_add(&connection->recv_queue, message);

            pthread_mutex_unlock(&connection->recv_mutex);
            pthread_cond_signal(&connection->recv_cond);

        } else free(buffer);

    } while (connection->alive == 1);

    return NULL;

}

static void *rtp_send_thread(void *void_ptr) {

    rtp_connection_t *connection = (rtp_connection_t *) void_ptr;
    message_t *message;
    int array_length = 0;
    int i;
    packet_t *packet_array;

    do {
        /* Extract the next message from the send queue */
        pthread_mutex_lock(&connection->send_mutex);
        while (queue_size(&connection->send_queue) == 0 && connection->alive == 1) {
            pthread_cond_wait(&connection->send_cond, &connection->send_mutex);
        }

        if (connection->alive == 0) break;

        message = queue_extract(&connection->send_queue);

        pthread_mutex_unlock(&connection->send_mutex);

        /* Packetize the message and send it */
        packet_array = packetize(message->buffer, message->length, &array_length);

        for (i = 0; i < array_length; i++) {

            /* Start sending the packetized messages */
            if (net_send_packet(connection->net_connection_handle, &packet_array[i]) <= 0) {
                /* remote side has disconnected */
                connection->alive = 0;
                break;
            }

            /*  ----FIX ME: Part II-D ---- 
             * 
             *  1. wait for the recv thread to notify you of when a NACK or
             *     an ACK has been received
             *  2. check the data structure for this connection to determine
             *     if an ACK or NACK was received.  (You'll have to add the
             *     necessary fields yourself)
             *  3. If it was an ACK, continue sending the packets.
             *  4. If it was a NACK, resend the last packet
             */

            pthread_mutex_lock(&connection->ack_mutex);

            while(connection->acknowledged == 0) {
                pthread_cond_wait(&connection->ack_cond, &connection->ack_mutex);
            }

            if(connection->acknowledged == 1) {     //NACK received
                i--;
            }

            connection->acknowledged = 0;

            pthread_mutex_unlock(&connection->ack_mutex);
        }

        free(packet_array);
        free(message->buffer);
        free(message);
    } while (connection->alive == 1);
    return NULL;


}

static rtp_connection_t *rtp_init_connection(int net_connection_handle) {
    rtp_connection_t *rtp_connection = malloc(sizeof(rtp_connection_t));

    if (rtp_connection == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }

    rtp_connection->net_connection_handle = net_connection_handle;

    queue_init(&rtp_connection->recv_queue);
    queue_init(&rtp_connection->send_queue);

    pthread_mutex_init(&rtp_connection->ack_mutex, NULL);
    pthread_mutex_init(&rtp_connection->recv_mutex, NULL);
    pthread_mutex_init(&rtp_connection->send_mutex, NULL);
    pthread_cond_init(&rtp_connection->ack_cond, NULL);
    pthread_cond_init(&rtp_connection->recv_cond, NULL);
    pthread_cond_init(&rtp_connection->send_cond, NULL);

    rtp_connection->alive = 1;

    pthread_create(&rtp_connection->recv_thread, NULL, rtp_recv_thread,
                   (void *) rtp_connection);
    pthread_create(&rtp_connection->send_thread, NULL, rtp_send_thread,
                   (void *) rtp_connection);

    return rtp_connection;
}

/* ================================================================ */
/*                           R T P    A P I                         */
/* ================================================================ */

rtp_connection_t *rtp_connect(char *host, int port) {

    int net_connection_handle;

    if ((net_connection_handle = net_connect(host, port)) < 1)
        return NULL;

    return (rtp_init_connection(net_connection_handle));
}

int rtp_disconnect(rtp_connection_t *connection) {

    message_t *message;
    packet_t term;

    term.type = TERM;
    term.payload_length = term.checksum = 0;
    net_send_packet(connection->net_connection_handle, &term);
    connection->alive = 0;

    net_disconnect(connection->net_connection_handle);
    pthread_cond_signal(&connection->send_cond);
    pthread_cond_signal(&connection->recv_cond);
    pthread_join(connection->send_thread, NULL);
    pthread_join(connection->recv_thread, NULL);
    net_release(connection->net_connection_handle);

    /* emtpy recv queue and free allocated memory */
    while ((message = queue_extract(&connection->recv_queue)) != NULL) {
        free(message->buffer);
        free(message);
    }
    queue_release(&connection->recv_queue);

    /* emtpy send queue and free allocated memory */
    while ((message = queue_extract(&connection->send_queue)) != NULL) {
        free(message);
    }
    queue_release(&connection->send_queue);

    free(connection);

    return 1;

}

int rtp_recv_message(rtp_connection_t *connection, char **buffer, int *length) {

    message_t *message;

    if (connection->alive == 0)
        return -1;
    /* lock */
    pthread_mutex_lock(&connection->recv_mutex);
    while (queue_size(&connection->recv_queue) == 0 && connection->alive == 1) {
        pthread_cond_wait(&connection->recv_cond, &connection->recv_mutex);
    }

    if (connection->alive == 0) {
        pthread_mutex_unlock(&connection->recv_mutex);
        return -1;
    }

    /* extract */
    message = queue_extract(&connection->recv_queue);
    *buffer = message->buffer;
    *length = message->length;
    free(message);

    /* unlock */
    pthread_mutex_unlock(&connection->recv_mutex);

    return *length;
}

int rtp_send_message(rtp_connection_t *connection, char *buffer, int length) {

    message_t *message;

    if (connection->alive == 0)
        return -1;

    message = malloc(sizeof(message_t));
    if (message == NULL) {
        return -1;
    }
    message->buffer = malloc((size_t) length);
    message->length = length;

    if (message->buffer == NULL) {
        free(message);
        return -1;
    }

    memcpy(message->buffer, buffer, (size_t) length);

    /* lock */
    pthread_mutex_lock(&connection->send_mutex);

    /* add */
    queue_add(&(connection->send_queue), message);

    /* unlock */
    pthread_mutex_unlock(&connection->send_mutex);
    pthread_cond_signal(&connection->send_cond);
    return 1;

}
