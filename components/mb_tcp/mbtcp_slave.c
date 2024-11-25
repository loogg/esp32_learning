#include "mbtcp_session.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/socket.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG "mbtcp_slave"
#include "esp_log.h"

#define MBTCP_THREAD_PRI        16
#define MBTCP_THREAD_STACK_SIZE 4096
#define MBTCP_LISTEN_BACKLOG    6
#define MBTCP_LISTEN_PORT       502

typedef struct _mbtcp_server {
    int socket;
    int reset;
} mbtcp_server_t;

static mbtcp_server_t _server = {0};

static int mbtcp_listen_create(void) {
    int enable = 1;
    int flags = 0;
    struct sockaddr_in addr;

    _server.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server.socket < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "create server socket failed");
        return -1;
    }

    if (setsockopt(_server.socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable, sizeof(enable)) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "socket %d setsockopt REUSEADDR failed", _server.socket);
        close(_server.socket);
        _server.socket = -1;
        return -1;
    }

    flags = fcntl(_server.socket, F_GETFL);
    if (fcntl(_server.socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "fcntl server socket %d O_NONBLOCK failed", _server.socket);
        close(_server.socket);
        _server.socket = -1;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MBTCP_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(_server.socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "bind server socket %d failed", _server.socket);
        close(_server.socket);
        _server.socket = -1;
        return -1;
    }

    if (listen(_server.socket, MBTCP_LISTEN_BACKLOG) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "listen server socket %d failed", _server.socket);
        close(_server.socket);
        _server.socket = -1;
        return -1;
    }

    ESP_LOGI(LOG_LOCAL_TAG, "server socket %d listen on port %d", _server.socket, MBTCP_LISTEN_PORT);

    return 0;
}

static void mbtcp_entry(void *param) {
    fd_set readset, exceptset;
    struct timeval select_timeout;

_server_start:
    vTaskDelay(pdMS_TO_TICKS(200));

    if (mbtcp_listen_create() < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "server listen create failed");
        goto _server_restart;
    }

    while (1) {
        select_timeout.tv_sec = 1;
        select_timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_ZERO(&exceptset);

        FD_SET(_server.socket, &readset);
        FD_SET(_server.socket, &exceptset);

        int maxfd = mbtcp_all_sessions_set_fds(&readset, &exceptset);
        if (maxfd < _server.socket + 1) maxfd = _server.socket + 1;

        int rc = select(maxfd, &readset, NULL, &exceptset, &select_timeout);

        if (_server.reset) {
            ESP_LOGI(LOG_LOCAL_TAG, "server reset");
            break;
        }

        mbtcp_session_check_timeout();

        if (rc == 0) {
            mbtcp_session_check_recv_timeout(NULL);
            continue;
        } else if (rc < 0) {
            break;
        }

        /* server error */
        if (FD_ISSET(_server.socket, &exceptset)) {
            ESP_LOGW(LOG_LOCAL_TAG, "server socket except");
            break;
        }

        /* server read */
        if (FD_ISSET(_server.socket, &readset)) {
            struct sockaddr_storage cliaddr;
            socklen_t clilen = sizeof(cliaddr);
            memset(&cliaddr, 0, sizeof(cliaddr));

            int client_fd = accept(_server.socket, (struct sockaddr *)&cliaddr, &clilen);
            if (client_fd < 0) {
                ESP_LOGW(LOG_LOCAL_TAG, "accept client socket failed");
            } else {
                mbtcp_session_t *session = mbtcp_session_take();
                if (session == NULL) {
                    ESP_LOGW(LOG_LOCAL_TAG, "no free session, close");
                    close(client_fd);
                } else {
                    ESP_LOGI(LOG_LOCAL_TAG, "new client connected");
                    mbtcp_session_init(session, client_fd, &cliaddr, clilen);
                }
            }
        }

        mbtcp_session_handle(&readset, &exceptset);
    }

_server_restart:
    ESP_LOGW(LOG_LOCAL_TAG, "restart");

    mbtcp_session_close_all();

    if (_server.socket >= 0) {
        close(_server.socket);
        _server.socket = -1;
    }
    _server.reset = 0;

    vTaskDelay(pdMS_TO_TICKS(3000));
    goto _server_start;
}

int mbtcp_slave_reset(void) {
    _server.reset = 1;
    return 0;
}

int mbtcp_slave_start(void) {
    _server.socket = -1;

    xTaskCreate(mbtcp_entry, "mbtcp", MBTCP_THREAD_STACK_SIZE, NULL, MBTCP_THREAD_PRI, NULL);

    return 0;
}
