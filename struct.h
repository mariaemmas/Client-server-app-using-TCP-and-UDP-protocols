#ifndef _STRUCT_H_
#define _STRUCT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>    
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_ID_CLIENT_LEN       10
#define BUFSIZE                 200   // buffer length for read from standard input
#define CLMAXCONN               100   // maximum number of client in listening queue
#define MAX_TOPICS_SUBSCRIBED   50      // to check, or queue
#define MAX_MESSAGES_STORED     50      // to check, or queue
#define MAX_TOPIC_LEN           50      // 50 + \0
#define MAX_CONTENT_LEN         1500
#define UDP_MESSAGE_LEN         MAX_TOPIC_LEN + MAX_CONTENT_LEN + 1 // 1551
#define TCP_MESSAGE_LEN         UDP_MESSAGE_LEN + MAX_ID_CLIENT_LEN + 19

// STRUCTURES used to parse messages send by UDP clients or send between server and TCP clients
// 1. Structure for messages received by the server from UDP clients (format predefined) 
struct udp_message {    // only to read 1 message from udp_client
    char topic[MAX_TOPIC_LEN];
    uint8_t tip_date;   // 0 INT, 1 SHORT_REAL, 2 FLOAT, 3 STRING
    char continut [MAX_CONTENT_LEN];
};

// 2. Structure for messages to send between server and TCP client 
struct tcp_message {                    // my tcp protocol header + payload
    // added to send tcp messages from subscriber
    uint8_t content_type;               // 4 - command (from client)
                                        // 1 - message (to client)
                                        // 2 - ask client to close (from server, same as recv with 0 bytes)
                                        // 3 - from subscriber - connect to server (trebuie? nu am folosit)
    char id_client[MAX_ID_CLIENT_LEN];  // 10 - used only when TCP client connects to server (but can be used for checks)
    uint8_t sf;                         // store-and-forward:           (from client)
    // from udp client
    uint8_t server_port;                // to be listed by tcp client (udp port)
    char client_udp_ip[16];             // to be listed by tcp client (udp)
    // from struct udp_message
    char topic[MAX_TOPIC_LEN];          // to complete the information  (from client)
    uint8_t tip_date;                   // 0 INT, 1 SHORT_REAL, 2 FLOAT, 3 STRING
    char continut [1500];               // payload - depends on content_type
                                        // content_type = 0 -> subscribe / unsubscribe command
                                        // content_type = 1 -> message from UDP client
};

// STRUCTURES used by the server
// 1. Structure with information about TCP clients (connected or disconnected)
struct client_tcp {
    char id_client[MAX_ID_CLIENT_LEN];      // 10
    int fd_socket;                          // current fd socket for client (use it if connected)
    uint8_t connected;                      // 1 if connected, 0 if disconnected
    uint16_t topic_count;                   // number of topics subscribed
    uint16_t topics[MAX_TOPICS_SUBSCRIBED]; // store topic id
    uint8_t sf[MAX_TOPICS_SUBSCRIBED];      // 1 - store messages if client offline
};

// 2. Structure for topics
struct topic {          // to simplify client_tcp structure
    uint16_t topic_id;
    char topic[MAX_TOPIC_LEN];
};

// 3. Structure for stored messages 
struct message_stored {
    uint16_t index_msg;                     // index pentru un mesaj
    char message[TCP_MESSAGE_LEN];
    uint16_t topic_id;
};

// 4. Structure to store index messages for disconnected clients
struct message_for {
    uint16_t index_msg;                 // index message
    char id_client[MAX_ID_CLIENT_LEN];  // id client
};


/* @details Transforms a string into a tcp_message structure 
 *
*/
void compose_tcp_message(struct tcp_message *tcp_msg, char *buf_message);

/* @details Transforms a tcp_message structure into a string 
 *
*/
void parse_tcp_message(struct tcp_message *tcp_msg, char *buf_message);

#endif