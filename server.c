#include "struct.h"

int check_port_number(int server_port) {
   if (server_port < 1024 || server_port >= 65535) {
        char buffer[BUFSIZE];
        snprintf (buffer, sizeof(buffer), "Port number must be between 1024 and 65535, received %d",server_port);
        perror(buffer);
        return -1;
    }
    return 0;
}

void parse_udp_msg(char *buff_udp_msg, struct udp_message *udp_msg) {
    memset(udp_msg, 0, sizeof(struct udp_message));
    memcpy(udp_msg->topic, buff_udp_msg, MAX_TOPIC_LEN);
    memcpy(&udp_msg->tip_date, buff_udp_msg + MAX_TOPIC_LEN, sizeof(uint8_t));
    memcpy(udp_msg->continut, buff_udp_msg + MAX_TOPIC_LEN + sizeof(uint8_t), MAX_CONTENT_LEN);
}

void close_sockets(int socket_server_tcp, int socket_server_udp, int clienti_tcp_nr, struct client_tcp *clienti_tcp) {
    struct tcp_message tcp_to_send;
    memset(&tcp_to_send, 0, TCP_MESSAGE_LEN);
    tcp_to_send.content_type = 2;
    // close TCP clients
    for (int i = 0; i < clienti_tcp_nr; i++) {
        if(send(clienti_tcp[i].fd_socket, &tcp_to_send, TCP_MESSAGE_LEN, 0) < 0) {
            perror("Send message to client to  disconnect - error");
        }
    }
    close(socket_server_udp);
    close(socket_server_tcp);
}

int check_or_add_topic(struct topic *topics, char *topic_target,
    uint16_t* topics_defined, uint8_t action) {
    // return topic_id
    // action = 0 -> add or find topic
    // action = 1 -> find topic (only)
    // action = 2 -> delete topic (check if i may)
    if ((action == 0) || (action == 1)) {
        for (int i = 0; i < *topics_defined; i++) {
            if (strcmp(topics[i].topic,  topic_target) == 0) {
                return topics[i].topic_id; // find
            }
        }
        if (action == 1) {
            return 0;       // don't find topic
        }
        if (*topics_defined == MAX_TOPICS_SUBSCRIBED) {
            return -1;       // cannot add, error
        }
        // add  
        strcpy(topics[*topics_defined].topic, topic_target);
        topics[*topics_defined].topic_id = (*topics_defined) + 1;
        uint16_t xreturn = (*topics_defined) + 1;
        (*topics_defined)++;
        return xreturn;
    }
    return 0;
}

void test_exit(int socket_server_tcp, int socket_server_udp, int clienti_tcp_nr, struct client_tcp *clienti_tcp) {
    char* buf = malloc(BUFSIZE * sizeof(char));
    if (buf == NULL) {
        perror("Memory not allocated for read buffer\n");
        exit(1);
    }
    fgets(buf, BUFSIZE, stdin);
    int lenbuf = strlen(buf);
    if (lenbuf < 0) {
        perror("Read from standard input error\n");
        free(buf);
        exit(1);
    } 
    if (!strncmp(buf, "exit", 5) == 0) {
        free(buf);
        // close server
        close_sockets(socket_server_tcp, socket_server_udp, clienti_tcp_nr, clienti_tcp);
        exit(0);
    }
    free(buf);
    perror("Unknown command from STDIN (one cmd accepted : exit\n");
    exit(1);
}

int main(int argc, char *argv[]){
    /* 0. read parameter */
    if (argc != 2){
        perror("Start the server using command ./server <PORT_DORIT>");
        return -1;
    }
    int server_port = atoi(argv[1]);
    if (check_port_number(server_port) < 0) {
        return -1;
    };

    // deactivate buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    /* define structure for tcp clients */
    struct client_tcp clienti_tcp[CLMAXCONN];
    int clienti_tcp_nr = 0;

    /* define structure for topics */
    struct topic topics[MAX_TOPICS_SUBSCRIBED];
    uint16_t topics_defined = 0;

    /* define structure for stored messages */
    struct message_stored messages_stored[MAX_MESSAGES_STORED];
    int messages_stored_no = 0;
    struct message_for messages_for[500];
    int messages_for_no = 0;
    int messages_stored_idx = 1;

    /* 1. create SOCKET - get fd for server sockets */
    int socket_server_tcp, socket_server_udp; //, client_sock, client_size;
 
    socket_server_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_server_tcp < 0){
        perror("Error while creating TCP socket for server\n");
        return -1;
    }

    socket_server_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_server_udp < 0){
        perror("Error while creating UDP socket for server\n");
        return -1;
    }

    /* 2. BIND */
    // Once you have a socket, you might have to associate that socket with a port on your local machine.
    // This is commonly done if you’re going to listen() for incoming connections on a specific port

    // the struct sockaddr holds socket address information for many types of sockets.
    struct sockaddr_in server_addr;
   
    /*clean buffers and structures*/
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    /* Set port and IP that we'll be listening for, any other IP_SRC or port will be dropped */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;   // use my IPv4 address

    // "toate comenzile si mesajele trimise ı̂n aplicatie trebuie să isi producă efectul imediat, motiv pentru"
    // "care trebuie să aveti, in vedere dezactivarea algoritmului Nagle (hint: TCP_NODELAY)"
    int yes=1;
    if (setsockopt(socket_server_tcp,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(yes)) == -1) {
        perror("Cannot deactivate Nagle algorithm\n");
        return -1;
    } 

    /* Bind to the set port and IP */
    if(bind(socket_server_tcp, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Couldn't bind (associate) TCP socket with given port\n");
        return -1;
    }

    /* 3. Listen for TCP clients */
    if (listen(socket_server_tcp, CLMAXCONN) < 0) {
        perror("Error while listening on TCP port\n");
        return -1;
    }

    if(bind(socket_server_udp, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Couldn't bind (associate) UDP socket with given port\n");
        return -1;
    }
 
    /* Add fd to the set */
    fd_set master_fds;   // master file descriptor list
    FD_ZERO(&master_fds);
    FD_SET(0, &master_fds);    // read from standard input
    FD_SET(socket_server_udp, &master_fds);
    FD_SET(socket_server_tcp, &master_fds);
    int fdmax;  // maximum file descriptor number
    if (socket_server_tcp > socket_server_udp) {
        fdmax = socket_server_tcp; 
    }
    else {
        fdmax = socket_server_udp; 
    }

    /* sockaddr_in for client - socket address information*/
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addr_len = sizeof(client_addr);    // accept

    /* structures to receive data */
    struct udp_message udp_msg;
    struct tcp_message tcp_cmd, tcp_to_send;
    char buff_tcp_msg[TCP_MESSAGE_LEN]; // to form messages to subscriber
    char buff_udp_msg[UDP_MESSAGE_LEN];

    ssize_t tcp_msg_nr_bytes_received;

    for(;;) {
        fd_set read_fds = master_fds;    // copy to temporary fds
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Cannot Select connection\n");
            return -1;
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { 
                // cases:
                // read from standard input
                // new tcp client connection
                // disconnect tcp client connection
                // use tcp client connection - subscribe command
                // use tcp client connection - unsubscribe command
                // udp connection
                if (i == 0) {       // read from standard input
                    test_exit(socket_server_tcp, socket_server_udp, clienti_tcp_nr, clienti_tcp);
                }

                // TCP connection
                if (i == socket_server_tcp) {   // tcp connection
                    // socket_client_tcp # socket_server_tcp !!!!
                    int socket_client_tcp = accept(socket_server_tcp, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (socket_client_tcp < 0) {
                        perror("Server : accept TCP client error\n");
                        continue;
                    }

                    int yes=1;
                    if (setsockopt(socket_client_tcp,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(yes)) == -1) {
                        perror("Cannot deactivate Nagle algorithm\n");
                        return -1;
                    }

                    struct client_tcp client;                   
                    memset(&client, 0, sizeof(client));
                    memset(&tcp_cmd, 0, TCP_MESSAGE_LEN);

                    tcp_msg_nr_bytes_received =  recv(socket_client_tcp, &tcp_cmd, TCP_MESSAGE_LEN, 0);
                    if(tcp_msg_nr_bytes_received < 0) {
                        perror("Couldn't receive \n");
                        continue;
                    }
                    memcpy(client.id_client, tcp_cmd.id_client, sizeof(tcp_cmd.id_client));
                    client.fd_socket = socket_client_tcp;
                    // check fd and client id -> to be unique

                    int found = -1; 
                    for (int j = 0; j < clienti_tcp_nr; j++) {
                        if (strcmp(clienti_tcp[j].id_client, client.id_client) == 0) {
                            if (clienti_tcp[j].connected == 0) {  // client disconnected
                                clienti_tcp[j].connected = 1;     // connect
                                clienti_tcp[j].fd_socket = socket_client_tcp;
                                char buffer[BUFSIZE];
                                snprintf (buffer, sizeof(buffer), "New client %s connected from %s:%d.",client.id_client, inet_ntoa(client_addr.sin_addr), server_port);
                                printf("%s\n", buffer);
                                found = j;
                                // send stored messages
                                for (int k = 0; k < messages_for_no; k++) {
                                    if (strcmp(messages_for[k].id_client, client.id_client) == 0) {
                                        uint16_t idx_msg = messages_for[k].index_msg;
                                        messages_for[k].index_msg = -1;             // prepare for delete
                                        if (idx_msg >=0) {
                                            for (int t = 0; t < messages_for_no; t++) {
                                                if (messages_stored[t].index_msg == idx_msg) {
                                                    if(send(clienti_tcp[j].fd_socket, &messages_stored[t].message, TCP_MESSAGE_LEN, 0) < 0) {
                                                        perror("Send message error for disconnected client");
                                                    }                                                                                                
                                                }
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                            else {
                                close(socket_client_tcp);           // if client already connected it already has a socket
                                char buffer[BUFSIZE];
                                snprintf (buffer, sizeof(buffer), "Client %s already connected.",client.id_client);
                                printf("%s\n", buffer);
                                found = -2;
                                break;
                            }
                        }
                    }
                    if (found == -2) {
                        // disconnect client 
                        continue;
                    }
                    if (found == -1) {           // client nou
                        client.connected = 1;
                        client.topic_count = 0;
                        memcpy(&clienti_tcp[clienti_tcp_nr], &client, sizeof(client));
                        clienti_tcp_nr++;
                        char buffer[BUFSIZE];
                        snprintf (buffer, sizeof(buffer), "New client %s connected from %s:%d.",client.id_client, inet_ntoa(client_addr.sin_addr), server_port);
                        printf("%s\n", buffer);
					}
                    FD_SET(socket_client_tcp, &master_fds);
                    if (fdmax < socket_client_tcp) {
                        fdmax = socket_client_tcp;
                    }
                    continue;
                }
                
               if (i != socket_server_tcp && i != socket_server_udp) {   // tcp connection                

                    // receive command from TCP client
                    memset(&tcp_cmd, 0, TCP_MESSAGE_LEN);
                    tcp_msg_nr_bytes_received =  recv(i, &tcp_cmd, TCP_MESSAGE_LEN, 0);
                    if (tcp_msg_nr_bytes_received < 0) {    // error
                        perror("Receive command from TCP client error \n");
                        continue;
                    }
                    int client_index = -1;
                    for (int j = 0; j < clienti_tcp_nr; j++) {
                        if (clienti_tcp[j].fd_socket == i) {
                            client_index = j;
                            break;
                        }
                    }
                    if (client_index == -1) {
                        perror("Client not found (to subscribe/unsubscribe), bad programmer\n");
                    }

                    if (tcp_msg_nr_bytes_received == 0) {
                        close(i);
                        FD_CLR(i, &master_fds); // dead socket...
                        // idea : to reevaluate fdmax, maybe if time
                        // disconnect client - find by fd_socket, alias i
                        clienti_tcp[client_index].connected = 0; // disconnected
                        char buffer[BUFSIZE];
                        snprintf (buffer, sizeof(buffer), "Client %s disconnected.", clienti_tcp[client_index].id_client);
                        printf("%s\n", buffer);
                        continue;
                    }
                    // nr topicuri subscrise de catre clientul client_index
                    int cl_topic_count = clienti_tcp[client_index].topic_count;
                    if (tcp_cmd.content_type != 4) {
                        // nu e comanda, nu interpretez
                        continue;
                    }     
                    if (strcmp(tcp_cmd.continut,"subscribe")  == 0) {        // subscribe
                        int topic_id_ret = check_or_add_topic(topics, tcp_cmd.topic, &topics_defined, 0);
                        if (topic_id_ret == -1 ) {
                            perror("MAX_TOPICS_SUBSCRIBED reached, please increase number\n");
                            continue;
                        }
                        // check if topic is already subscribed
                        int already_subscribed = -1;
                        for (int j = 0; j < cl_topic_count; j++) {
                            if (clienti_tcp[client_index].topics[j] == topic_id_ret) {
                                clienti_tcp[client_index].sf[j] = tcp_cmd.sf;
                                already_subscribed = j;
                                break;
                            }
                        }
                        if (already_subscribed == -1) {      // must add
                            clienti_tcp[client_index].sf[cl_topic_count] = tcp_cmd.sf;
                            clienti_tcp[client_index].topics[cl_topic_count] = topic_id_ret;
                            clienti_tcp[client_index].topic_count++;
                        }
                        continue;
                    }
                    if (strcmp(tcp_cmd.continut,"unsubscribe") == 1) {        // unsubscribe
                        int topic_id_ret = check_or_add_topic(topics, tcp_cmd.topic, &topics_defined, 1);
                        if (topic_id_ret > 0) {
                            int is_subscribed = 0;
                            for (int j = 0; j < cl_topic_count; j++) {
                                if (clienti_tcp[client_index].topics[j] == topic_id_ret) {                                
                                    clienti_tcp[client_index].topic_count--;
                                    is_subscribed = j;
                                    if (j == cl_topic_count -1) {
                                        break;
                                    }
                                    clienti_tcp[client_index].sf[j] = clienti_tcp[client_index].sf[cl_topic_count - 1];
                                    clienti_tcp[client_index].topics[j] = clienti_tcp[client_index].topics[cl_topic_count - 1];
                                    break;
                                }
                            }
                            if (is_subscribed == 0) {
                                perror("asked to unsubscribe topic not subscribed !! Ckeck. \n");
                            }
                            continue;
                        }
                        else {
                            perror("Asked to unsubscribe inexistent topic. Maybe something wrong.\n");
                        }
                        continue;
                    }
               }

                // UDP connection
                if (i == socket_server_udp) {
                    memset(&buff_udp_msg, 0 , UDP_MESSAGE_LEN);
                    if (recvfrom(i, buff_udp_msg, UDP_MESSAGE_LEN, 0, (struct sockaddr*)&client_addr, &client_addr_len) < 0) {
                        perror("Receive error from UDP client");
                        continue;
                    }
                    parse_udp_msg(buff_udp_msg, &udp_msg);

                    // store topic in topics[] if new
                    int topic_id_ret = check_or_add_topic(topics, udp_msg.topic, &topics_defined, 0);

                    if (topic_id_ret == -1 ) {
                        perror("MAX_TOPICS_SUBSCRIBED reached, please increase number\n");
                        continue;
                    }

                    // formez mesajul (sf e optiune de client...)
                    memset(&tcp_to_send, 0, TCP_MESSAGE_LEN);
                    tcp_to_send.content_type = 1;       // message
                    tcp_to_send.server_port = ntohs(client_addr.sin_port);
                    strcpy(tcp_to_send.client_udp_ip, inet_ntoa(client_addr.sin_addr)); 
                    memcpy(tcp_to_send.topic, udp_msg.topic, MAX_TOPIC_LEN);
                    tcp_to_send.tip_date = udp_msg.tip_date;
                    memcpy(tcp_to_send.continut, udp_msg.continut, MAX_CONTENT_LEN);

                    // send message from connected clients
                    // store message for disconnected clients if sf=1

                    int disconn_client_with_sf = 0;
                    for (int j = 0; j < clienti_tcp_nr; j++) {
                        for (int k = 0; k <= clienti_tcp[j].topic_count; k++) {
                            if (clienti_tcp[j].topics[k] == topic_id_ret) {
                                // client with subscription to topic
                            memcpy(tcp_to_send.id_client,clienti_tcp[j].id_client, 10);
                            tcp_to_send.sf = clienti_tcp[j].sf[k];
                                tcp_to_send.sf = clienti_tcp[j].sf[k];
                                memcpy(tcp_to_send.id_client, clienti_tcp[j].id_client, MAX_ID_CLIENT_LEN);
                                compose_tcp_message(&tcp_to_send, buff_tcp_msg);

                                if (clienti_tcp[j].connected == 1) {
                                    // client is connected, send message
                                    if(send(clienti_tcp[j].fd_socket, &buff_tcp_msg, TCP_MESSAGE_LEN, 0) < 0) {
                                        perror("Send message error");
                                    }
                                }
                                if (clienti_tcp[j].connected == 0) {
                                    // client is disconnected
                                    if (clienti_tcp[j].sf[k] == 1) {
                                        strcpy(messages_for[messages_for_no].id_client, tcp_to_send.id_client);
                                        messages_for[messages_for_no].index_msg = messages_stored_idx;
                                        messages_for_no++;
                                        // store message
                                        disconn_client_with_sf++;
                                    }
                                }                                    
                            }
                        }
                    }
                    if (disconn_client_with_sf > 0) {
                        // store message for disconnected clients
                        compose_tcp_message(&tcp_to_send, buff_tcp_msg);
                        memcpy(&messages_stored[messages_stored_no].message, &buff_tcp_msg,TCP_MESSAGE_LEN);
                        messages_stored[messages_stored_no].topic_id = topic_id_ret;
                        messages_stored[messages_stored_no].index_msg = messages_stored_idx;
                        messages_stored_no++;
                        messages_stored_idx++;
                    }
                    continue;
                }
            }
        }
    }
    /* Close sockets */
    close_sockets(socket_server_tcp, socket_server_udp, clienti_tcp_nr, clienti_tcp);
    return 0;
}

