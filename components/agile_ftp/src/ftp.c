#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/socket.h>
#include "ftp_session.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG   "ftp"
#include "esp_log.h"

#ifndef FTP_DEFAULT_PORT
#define FTP_DEFAULT_PORT 21
#endif

#define FTP_LISTEN_BACKLOG 6

static int ftp_port = FTP_DEFAULT_PORT;
static uint8_t force_restart = 0;
static int server_fd = -1;

#define RT_EOK   0
#define RT_ERROR 1

int ftp_force_restart(void) {
    force_restart = 1;
    return RT_EOK;
}

int ftp_get_port(void) { return ftp_port; }

int ftp_set_port(int port) {
    if ((port <= 0) || (port > 65535)) return -RT_ERROR;
    ftp_port = port;
    return RT_EOK;
}

static int ftp_listen_create(void) {
    int enable = 1;
    int flags = 0;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "create server socket failed");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable, sizeof(enable)) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "socket %d setsockopt REUSEADDR failed", server_fd);
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    flags = fcntl(server_fd, F_GETFL);
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "fcntl server socket %d O_NONBLOCK failed", server_fd);
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ftp_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "bind server socket %d failed", server_fd);
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    if (listen(server_fd, FTP_LISTEN_BACKLOG) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "listen server socket %d failed", server_fd);
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    ESP_LOGI(LOG_LOCAL_TAG, "server socket %d listen on port %d", server_fd, ftp_port);

    return 0;
}

static void ftp_entry(void *parameter) {
    fd_set readset, exceptset;
    struct timeval select_timeout;

_ftp_start:
    vTaskDelay(pdMS_TO_TICKS(200));

    if (ftp_listen_create() < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "server listen create failed");
        goto _ftp_restart;
    }

    while (1) {
        if (force_restart) {
            force_restart = 0;
            ftp_session_force_quit();
            break;
        }

        select_timeout.tv_sec = 1;
        select_timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_ZERO(&exceptset);

        FD_SET(server_fd, &readset);
        FD_SET(server_fd, &exceptset);

        int rc = select(server_fd + 1, &readset, NULL, &exceptset, &select_timeout);
        if (rc < 0) break;
        if (rc > 0) {
            if (FD_ISSET(server_fd, &exceptset)) break;
            if (FD_ISSET(server_fd, &readset)) {
                struct sockaddr_storage cliaddr;
                socklen_t clilen = sizeof(cliaddr);
                memset(&cliaddr, 0, sizeof(cliaddr));

                int client_fd = accept(server_fd, (struct sockaddr *)&cliaddr, &clilen);
                if (client_fd < 0) {
                    ESP_LOGW(LOG_LOCAL_TAG, "accept client socket failed");
                } else {
                    if (ftp_session_create(client_fd, &cliaddr, clilen) != RT_EOK) close(client_fd);
                }
            }
        }
    }

_ftp_restart:
    ESP_LOGW(LOG_LOCAL_TAG, "service go wrong, now wait restarting...");
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    goto _ftp_start;
}

int ftp_init(uint32_t stack_size, uint8_t priority) {
    xTaskCreate(ftp_entry, "ftp", stack_size, NULL, priority, NULL);

    printf("\r\n[FTP] Powered by Ma Longwei\r\n");
    printf("[FTP] github: https://github.com/loogg\r\n");
    printf("[FTP] Email: 2544047213@qq.com\r\n");

    return RT_EOK;
}
