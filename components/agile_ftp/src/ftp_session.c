#include <sys/socket.h>
#include "ftp_session.h"
#include "ftp_session_cmd.h"

#ifndef FTP_MAX_SESSION_NUM
#define FTP_MAX_SESSION_NUM 10
#endif

#ifndef FTP_SESSION_USERNAME
#define FTP_SESSION_USERNAME "loogg"
#endif

#ifndef FTP_SESSION_PASSWORD
#define FTP_SESSION_PASSWORD "loogg"
#endif

#ifndef FTP_SESSION_WELCOME_MSG
#define FTP_SESSION_WELCOME_MSG "220 -= welcome on RT-Thread FTP server =-\r\n"
#endif

#ifndef FTP_SESSION_TIMEOUT
#define FTP_SESSION_TIMEOUT 3
#endif

#define RT_EOK   0
#define RT_ERROR 1

static int ftp_max_session_num = FTP_MAX_SESSION_NUM;
static char ftp_session_username[64] = FTP_SESSION_USERNAME;
static char ftp_session_password[64] = FTP_SESSION_PASSWORD;
static char ftp_session_welcome_msg[100] = FTP_SESSION_WELCOME_MSG;
static LIST_HEAD(listhead, ftp_session) _session_head = LIST_HEAD_INITIALIZER(_session_head);
static portMUX_TYPE _spinlock = portMUX_INITIALIZER_UNLOCKED;

int ftp_get_max_session_num(void) { return ftp_max_session_num; }

int ftp_set_max_session_num(int num) {
    if (num <= 0) return -RT_ERROR;

    ftp_max_session_num = num;
    return RT_EOK;
}

const char *ftp_get_session_username(void) { return ftp_session_username; }

int ftp_set_session_username(const char *username) {
    if (username == NULL) return -RT_ERROR;

    strncpy(ftp_session_username, username, sizeof(ftp_session_username) - 1);
    ftp_session_username[sizeof(ftp_session_username) - 1] = '\0';
    return RT_EOK;
}

const char *ftp_get_session_password(void) { return ftp_session_password; }

int ftp_set_session_password(const char *password) {
    if (password == NULL) return -RT_ERROR;

    strncpy(ftp_session_password, password, sizeof(ftp_session_password) - 1);
    ftp_session_password[sizeof(ftp_session_password) - 1] = '\0';
    return RT_EOK;
}

const char *ftp_get_session_welcome_msg(void) { return ftp_session_welcome_msg; }

int ftp_set_session_welcome_msg(const char *welcome_msg) {
    if (welcome_msg == NULL) return -RT_ERROR;

    strncpy(ftp_session_welcome_msg, welcome_msg, sizeof(ftp_session_welcome_msg) - 1);
    ftp_session_welcome_msg[sizeof(ftp_session_welcome_msg) - 1] = '\0';
    return RT_EOK;
}

static int ftp_session_get_num(void) {
    int num = 0;
    struct ftp_session *session;

    taskENTER_CRITICAL(&_spinlock);

    LIST_FOREACH(session, &_session_head, entries) { num++; }

    taskEXIT_CRITICAL(&_spinlock);

    return num;
}

static int ftp_session_delete(struct ftp_session *session) {
    taskENTER_CRITICAL(&_spinlock);
    LIST_REMOVE(session, entries);
    taskEXIT_CRITICAL(&_spinlock);

    close(session->fd);
    if (session->port_pasv_fd >= 0) close(session->port_pasv_fd);
    free(session);

    return RT_EOK;
}

static int ftp_session_read(struct ftp_session *session, uint8_t *buf, int bufsz, int timeout) {
    int bytes = 0;
    int rc = 0;

    if (bufsz <= 0) return bufsz;

    while (bytes < bufsz) {
        rc = recv(session->fd, &buf[bytes], (size_t)(bufsz - bytes), MSG_DONTWAIT);
        if (rc <= 0) return -1;

        bytes += rc;
        if (bytes >= bufsz) break;

        if (timeout > 0) {
            fd_set readset, exceptset;
            struct timeval interval;

            interval.tv_sec = timeout / 1000;
            interval.tv_usec = (timeout % 1000) * 1000;

            FD_ZERO(&readset);
            FD_ZERO(&exceptset);
            FD_SET(session->fd, &readset);
            FD_SET(session->fd, &exceptset);

            rc = select(session->fd + 1, &readset, NULL, &exceptset, &interval);
            if (rc < 0) return -1;
            if (rc == 0) break;
            if (FD_ISSET(session->fd, &exceptset)) return -1;
        } else
            break;
    }

    return bytes;
}

static int ftp_session_process(struct ftp_session *session, char *cmd_buf) {
    int result = RT_EOK;

    /* remove \r\n */
    char *ptr = cmd_buf;
    while (*ptr) {
        if ((*ptr == '\r') || (*ptr == '\n')) *ptr = 0;
        ptr++;
    }

    char *cmd = cmd_buf;
    char *cmd_param = strchr(cmd, ' ');
    if (cmd_param) {
        *cmd_param = '\0';
        cmd_param++;
    }

    switch (session->state) {
        case FTP_SESSION_STATE_USER: {
            if (strstr(cmd, "USER") != cmd) {
                char *reply = "502 Not Implemented.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                break;
            }

            if (strcmp(cmd_param, "anonymous") == 0) {
                session->is_anonymous = 1;
                char *reply = "331 anonymous login OK send e-mail address for password.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                session->state = FTP_SESSION_STATE_PASSWD;
                break;
            }

            if (strcmp(cmd_param, ftp_session_username) == 0) {
                session->is_anonymous = 0;
                char *reply = "331 Password required.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                session->state = FTP_SESSION_STATE_PASSWD;
                break;
            }

            char *reply = "530 Login incorrect. Bye.\r\n";
            send(session->fd, reply, strlen(reply), 0);
            result = -RT_ERROR;
        } break;

        case FTP_SESSION_STATE_PASSWD: {
            if (strstr(cmd, "PASS") != cmd) {
                char *reply = "502 Not Implemented.\r\n";
                send(session->fd, reply, strlen(reply), 0);
                break;
            }

            if (session->is_anonymous || (strcmp(cmd_param, ftp_session_password) == 0)) {
                char *reply = "230 User logged in\r\n";
                send(session->fd, reply, strlen(reply), 0);
                memset(session->currentdir, 0, sizeof(session->currentdir));
                snprintf(session->currentdir, sizeof(session->currentdir), "/sdcard");
                // session->currentdir[0] = '/';
                session->state = FTP_SESSION_STATE_PROCESS;
                break;
            }

            char *reply = "530 Login incorrect. Bye.\r\n";
            send(session->fd, reply, strlen(reply), 0);
            result = -RT_ERROR;
        } break;

        case FTP_SESSION_STATE_PROCESS: {
            int rc = ftp_session_cmd_process(session, cmd, cmd_param);
            if (rc != RT_EOK) {
                result = -RT_ERROR;
                break;
            }
        } break;

        default:
            result = -RT_ERROR;
            break;
    }

    session->tick_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(FTP_SESSION_TIMEOUT * 1000);

    return result;
}

static void ftp_client_entry(void *parameter) {
    struct ftp_session *session = parameter;
    int option = 1;
    int flags = 0;
    fd_set readset, exceptset;
    struct timeval select_timeout;

    int rc = setsockopt(session->fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&option, sizeof(int));
    if (rc < 0) goto _exit;

    flags = fcntl(session->fd, F_GETFL);
    fcntl(session->fd, F_SETFL, flags | O_NONBLOCK);

    session->port_pasv_fd = -1;
    session->is_anonymous = 0;
    session->offset = 0;
    session->state = FTP_SESSION_STATE_USER;
    session->tick_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(FTP_SESSION_TIMEOUT * 1000);

    char cmd_buf[1024];

    send(session->fd, ftp_session_welcome_msg, strlen(ftp_session_welcome_msg), 0);

    while (1) {
        if (session->force_quit) break;

        select_timeout.tv_sec = 1;
        select_timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_ZERO(&exceptset);

        FD_SET(session->fd, &readset);
        FD_SET(session->fd, &exceptset);

        rc = select(session->fd + 1, &readset, NULL, &exceptset, &select_timeout);
        if (rc < 0) break;
        if (rc > 0) {
            if (FD_ISSET(session->fd, &exceptset)) break;
            if (FD_ISSET(session->fd, &readset)) {
                int cmd_len = ftp_session_read(session, (uint8_t *)cmd_buf, sizeof(cmd_buf) - 1, 30);
                if (cmd_len <= 0) break;
                cmd_buf[cmd_len] = '\0';
                if (ftp_session_process(session, cmd_buf) != RT_EOK) break;
            }
        }

        if ((xTaskGetTickCount() - session->tick_timeout) < (portMAX_DELAY / 2)) break;
    }

_exit:
    ftp_session_delete(session);
    vTaskDelete(NULL);
}

int ftp_session_create(int fd, struct sockaddr_storage *addr, socklen_t addr_len) {
    if (fd < 0) return -RT_ERROR;

    if (ftp_session_get_num() >= ftp_max_session_num) return -RT_ERROR;

    struct ftp_session *session = malloc(sizeof(struct ftp_session));
    if (session == NULL) return -RT_ERROR;
    memset(session, 0, sizeof(struct ftp_session));
    session->fd = fd;
    memcpy(&(session->remote), addr, addr_len);

    taskENTER_CRITICAL(&_spinlock);
    LIST_INSERT_HEAD(&_session_head, session, entries);
    taskEXIT_CRITICAL(&_spinlock);

    xTaskCreate(ftp_client_entry, "ftpc", 4096, session, 20, NULL);

    return RT_EOK;
}

int ftp_session_force_quit(void) {
    struct ftp_session *session;

    taskENTER_CRITICAL(&_spinlock);
    LIST_FOREACH(session, &_session_head, entries) { session->force_quit = 1; }
    taskEXIT_CRITICAL(&_spinlock);

    return RT_EOK;
}
