#include "struct.h"

// subscriber actions :
//      STDIN - subscribe : read info, SEND to server, MESSAGE
//      STDIN - unsubscribe : read info, SEND to server, MESSAGE
//      STDIN - exit : SEND to server info : client disconnected + close
//      read from port TCP : 
//          - RECV message from server, show MESSAGE
//          - RECV server disconnected (content_type=2), close 

int read_from_stdin(int socket_server_tcp, char* id_client) {
    char* buf = malloc(BUFSIZE * sizeof(char));
    if (buf == NULL) {
        perror("Client : Memory not allocated for read buffer\n");
        return -1;
    }
    memset(buf, '\0', BUFSIZE);
    fgets(buf, BUFSIZE, stdin);

    char* token = (char *)strtok(buf, " \n");
    if (strcmp(token, "exit") == 0) {
         free(buf);
         return 1;
    }

    struct tcp_message tcp_cmd;
    memset(&tcp_cmd, 0, TCP_MESSAGE_LEN);
    tcp_cmd.content_type = 4;
    int receive_cmd = 0;
    if (strcmp(token, "subscribe") == 0) {
        // subscribe <TOPIC> <SF>

        strcpy(tcp_cmd.continut, token);  // write: subscribe

        token = (char *)strtok(NULL, " \n");
        if (token == NULL) {
            perror("Incomplete subscribe command - no params\n");
            free(buf);
            return -1;
        }
        if (strlen(token) > 50) {
            perror("Incorrect subscribe command - topic too long\n");
            free(buf);
            return -1;
        }
        strcpy(tcp_cmd.topic, token);

        token = (char *)strtok(NULL, " \n");
        if (token == NULL) {
            perror("Incomplete subscribe command - no sf\n");
            free(buf);
            return -1;
        }
        if (strlen(token) != 1) {
            perror("Incomplete subscribe command - wrong sf length\n");
            free(buf);
            return -1;
        }
        if (strcmp(token,"0") == 0) {
            tcp_cmd.sf = 0;
        }
        else {
            if (strcmp(token,"1") == 0) {
                tcp_cmd.sf = 1;
            }
            else {
                perror("subscribe sf invalid\n");
                free(buf);
                return -1;
            }
        }
        receive_cmd=1;
        printf("Subscribed to topic.\n");
    }
    if (strcmp(token, "unsubscribe") == 0) {
        // unsubscribe <TOPIC>
        strcpy(tcp_cmd.continut, token);  // write: subscribe

        token = (char *)strtok(NULL, " \n");
        if (token == NULL) {
            perror("Incomplete unsubscribe command - no params\n");
            free(buf);
            return -1;
        }
        if (strlen(token) > 50) {
            perror("Incorrect unsubscribe command - topic too long\n");
            free(buf);
            return -1;
        }
        strcpy(tcp_cmd.topic, token);
        receive_cmd=1;
        printf("Unsubscribed to topic.\n");
    }
    if (receive_cmd == 1) {
        // send command to server
        memcpy(tcp_cmd.id_client, &id_client, MAX_ID_CLIENT_LEN);
        if (send(socket_server_tcp, &tcp_cmd, TCP_MESSAGE_LEN, 0) < 0) {
            perror("Client send subscribe/unsubscribe cmd to server error\n");
            return -1;
        }
    }
    free(buf);
    return 0;
}

int main(int argc, char *argv[]){
    /* 0. read parameter */
    if (argc != 4){
        perror("Start the subscriber using command ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>");
        return -1;
    }
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char id_client[MAX_ID_CLIENT_LEN]; // 10
    
    if (strlen(argv[1]) > MAX_ID_CLIENT_LEN) {
        perror("id_client parameter with length over 10 chars \n");
        return -1;
    }
    memset(&id_client, 0, MAX_ID_CLIENT_LEN);
    int lenargv1 = strlen(argv[1]);
    memcpy(&id_client, argv[1], lenargv1); //get id_client

    char buffy[10];
    memset(buffy, 0 ,sizeof(buffy));
    
    int server_port = atoi(argv[3]);              //get port
    if (server_port < 1024 || server_port >= 65535) {
        char buffer[BUFSIZE];
        snprintf (buffer, sizeof(buffer), "Port number must be between 1024 and 65535, received %d",server_port);
        perror(buffer);
        return -1;
    } 

    /* 1. create socket */
    int socket_server_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server_tcp < 0){
        perror("Error while creating TCP socket for client\n");
        return -1;
    }

    // disable Nagle 
    int yes=1;
    if (setsockopt(socket_server_tcp,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof yes) == -1) {
        perror("Cannot deactivate Nagle algorithm (client)\n");
        return -1;
    } 

    /* 2. connect */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_aton(argv[2], &server_addr.sin_addr) == 0) {
        perror("Received an incorrect IP address\n");
    };    
    if(connect(socket_server_tcp, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("[CLIENT] Unable to connect\n");
        return -1;
    }

    /* 3. send/receive */
    struct tcp_message tcp_cmd;
    char buff_tcp_msg[TCP_MESSAGE_LEN]; // to form messages to client
    tcp_cmd.content_type = 3;
    memcpy(tcp_cmd.id_client, id_client, MAX_ID_CLIENT_LEN);

    compose_tcp_message(&tcp_cmd, buff_tcp_msg);

    if(send(socket_server_tcp, &buff_tcp_msg, TCP_MESSAGE_LEN, 0) < 0) {
        perror("Send client id error\n");
    }
    fd_set master_fds;   // master file descriptor list
    FD_ZERO(&master_fds);
    FD_SET(0, &master_fds);    // read from standard input
    FD_SET(socket_server_tcp, &master_fds);
    int fdmax = socket_server_tcp; // maximum file descriptor number

    int tcp_msg_nr_bytes_received = 0;
    for(;;) {
        fd_set read_fds = master_fds;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Client : cannot Select connection\n");
            return -1;
        }

        // run through the existing connections
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { 
                // cases:
                // read from standard input
                //    subscribe, unsubscribe
                //    exit
                // new message from tcp connection
                if (i == 0) {       
                    // read from standard input and
                    // send subscription/unsubscription to server
                    int rez = read_from_stdin(socket_server_tcp, id_client);
                    if (rez == 1) {
                        close(socket_server_tcp);   // exit - disconnect
                        return 0;
                    }
                }
                if (i == socket_server_tcp) {
                    memset(&tcp_cmd, 0, TCP_MESSAGE_LEN);
                    memset(&buff_tcp_msg, 0, TCP_MESSAGE_LEN);
                    tcp_msg_nr_bytes_received = recv(socket_server_tcp, &buff_tcp_msg, TCP_MESSAGE_LEN, 0);
                    if (tcp_msg_nr_bytes_received < 0) {
                        perror("Client error receive message from server\n");
                        continue;
                    }
                    parse_tcp_message(&tcp_cmd, buff_tcp_msg);

                    if (tcp_cmd.content_type == 2) {
                        close(socket_server_tcp);   // server closed, client must close
                        exit(0);
                    }
                    if (tcp_msg_nr_bytes_received == 0) { // ATENTIE pot pune -99...
                        close(socket_server_tcp);   // server closed, client must close
                        exit(0);
                    }
                    printf("%s:",tcp_cmd.client_udp_ip);
                    printf("%d",tcp_cmd.server_port);
                    printf(" - %s - ", tcp_cmd.topic);
                    if (tcp_cmd.tip_date == 0) {
                        int sign = tcp_cmd.continut[0];
                        uint32_t uint_send;
                        memcpy(&uint_send, tcp_cmd.continut + 1, sizeof(uint32_t));
                        int int_send = ntohl(uint_send);
                        if (sign == 1) {
                            int_send = int_send * (-1);
                        }
                        printf("INT - %d\n", int_send);
                    }
                    if (tcp_cmd.tip_date == 1) {
                        uint16_t uint_send;
                        memcpy(&uint_send, tcp_cmd.continut + 0, sizeof(uint16_t));
                        int int_send = ntohs(uint_send);
                        printf("SHORT_REAL - %.2f\n", (float)int_send/100);
                    }
                    if (tcp_cmd.tip_date == 2) {
                        int sign = tcp_cmd.continut[0];
                        uint32_t uint_send;
                        memcpy(&uint_send, tcp_cmd.continut + 1, sizeof(uint32_t));
                        int int_send = ntohl(uint_send);
                        if (sign == 1) {
                            int_send = int_send * (-1);
                        }
                        uint8_t pow_send = tcp_cmd.continut[5];
                        int multiply=1;
                        for(i = 0; i < pow_send; i++) {
                            multiply *= 10;            
                        }
                        printf("FLOAT - %.4f\n", (float)int_send / multiply); 
                    }
                    if (tcp_cmd.tip_date == 3) {
                        printf("STRING - %s\n", tcp_cmd.continut);
                    }
                }
            }
        }
    }
    /* 4. close socket */
    close(socket_server_tcp);
}