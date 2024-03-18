#include "struct.h"

extern void compose_tcp_message(struct tcp_message *tcp_msg, char *buf_message) {
    memset(buf_message, 0, sizeof(TCP_MESSAGE_LEN));
//    memcpy(buf_message, tcp_msg, TCP_MESSAGE_LEN);
    memcpy(buf_message + 0, &tcp_msg->content_type, sizeof(uint8_t));
    memcpy(buf_message + 1, tcp_msg->id_client, MAX_ID_CLIENT_LEN);
    memcpy(buf_message + MAX_ID_CLIENT_LEN + 1, &tcp_msg->sf, sizeof(uint8_t));
    memcpy(buf_message + MAX_ID_CLIENT_LEN + 2, &tcp_msg->server_port, sizeof(uint8_t));
    memcpy(buf_message + MAX_ID_CLIENT_LEN + 3, &tcp_msg->client_udp_ip, 16);
    memcpy(buf_message + MAX_ID_CLIENT_LEN + 19, tcp_msg->topic, MAX_TOPIC_LEN);
    memcpy(buf_message + MAX_ID_CLIENT_LEN + MAX_TOPIC_LEN + 19, &tcp_msg->tip_date, sizeof(uint8_t));
    memcpy(buf_message + MAX_ID_CLIENT_LEN + MAX_TOPIC_LEN + 20, tcp_msg->continut, MAX_CONTENT_LEN);
}

extern void parse_tcp_message(struct tcp_message *tcp_msg, char *buf_message) {
    memset(tcp_msg, 0, sizeof(TCP_MESSAGE_LEN));
    memcpy(&tcp_msg->content_type, buf_message + 0, sizeof(uint8_t));
    memcpy(tcp_msg->id_client, buf_message + 1, MAX_ID_CLIENT_LEN);
    memcpy(&tcp_msg->sf, buf_message + MAX_ID_CLIENT_LEN + 1, sizeof(uint8_t));
    memcpy(&tcp_msg->server_port, buf_message + MAX_ID_CLIENT_LEN + 2, sizeof(uint8_t));
    memcpy(&tcp_msg->client_udp_ip, buf_message + MAX_ID_CLIENT_LEN + 3, 16);
    memcpy(tcp_msg->topic, buf_message + MAX_ID_CLIENT_LEN + 19, MAX_TOPIC_LEN);
    memcpy(&tcp_msg->tip_date, buf_message + MAX_ID_CLIENT_LEN + MAX_TOPIC_LEN + 19, sizeof(uint8_t));
    memcpy(tcp_msg->continut, buf_message + MAX_ID_CLIENT_LEN + MAX_TOPIC_LEN + 20, MAX_CONTENT_LEN);
}
