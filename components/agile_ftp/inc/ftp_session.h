#ifndef __FTP_SESSION_H
#define __FTP_SESSION_H

#include <stdint.h>
#include "sys/queue.h"
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum ftp_session_state
{
    FTP_SESSION_STATE_USER = 0,
    FTP_SESSION_STATE_PASSWD,
    FTP_SESSION_STATE_PROCESS
};

struct ftp_session
{
    int fd;
    int port_pasv_fd;
    int is_anonymous;
    int offset;
    struct sockaddr_storage remote;
    enum ftp_session_state state;
    char currentdir[260];
    uint8_t force_quit;
    TickType_t tick_timeout;
    LIST_ENTRY(ftp_session) entries;
};

int ftp_session_create(int fd, struct sockaddr_storage *addr, socklen_t addr_len);
int ftp_session_force_quit(void);

#endif
