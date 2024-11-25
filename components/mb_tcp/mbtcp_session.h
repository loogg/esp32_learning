#ifndef __MBTCP_SESSION_H
#define __MBTCP_SESSION_H

#include <stdint.h>
#include <agile_modbus.h>
#include "sys/queue.h"
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct _mbtcp_session {
    int active;
    int socket;
    struct sockaddr_storage cliaddr;
    TickType_t timeout;
    agile_modbus_tcp_t mb_ctx;
    uint8_t send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t data_step;
    uint16_t data_len;
    uint16_t len_to_read;
    TickType_t recv_timeout;
    LIST_ENTRY(_mbtcp_session) entries;
} mbtcp_session_t;

int mbtcp_all_sessions_set_fds(fd_set *readset, fd_set *exceptset);
mbtcp_session_t *mbtcp_session_take(void);
int mbtcp_session_init(mbtcp_session_t *session, int socket, struct sockaddr_storage *addr, socklen_t clilen);
void mbtcp_session_close(mbtcp_session_t *session);
void mbtcp_session_close_all(void);
void mbtcp_session_check_timeout(void);
void mbtcp_session_check_recv_timeout(mbtcp_session_t *session);
void mbtcp_session_handle(fd_set *readset, fd_set *exceptset);

#endif /* __MBTCP_SESSION_H */
