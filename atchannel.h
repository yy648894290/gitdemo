#ifndef _AT_CHANNEL_H
#define _AT_CHANNEL_H 1

int create_at_socket_channel();

int create_at_serial_channel();

/*
 * 此函数用于向AT串口发送数据并接收返回的所有数据
 * 接收超时：outime 单位：毫秒
 * 如果outime <= 0，则只发送不接收
 */
int COMD_AT_PROCESS ( char *send_buf, int outime, char* recv_buf, int recv_len );

int at_handshake();

#endif /* _AT_CHANNEL_H */


