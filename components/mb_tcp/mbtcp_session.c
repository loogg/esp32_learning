#include "mbtcp_session.h"
#include <sys/socket.h>
#include <sys/time.h>
#include "mb_slave.h"
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define LOG_LOCAL_TAG "mbtcp_session"
#include "esp_log.h"

#define MBTCP_MAX_SESSIONS         2
#define MBTCP_SESSION_TIMEOUT      60000
#define MBTCP_SESSION_RECV_TIMEOUT 100
#define MBTCP_DATA_BUFSZ           512

enum { STEP_HEAD = 0, STEP_MBAP, STEP_PDU, STEP_OK };

static LIST_HEAD(listhead, _mbtcp_session) _session_head = LIST_HEAD_INITIALIZER(_session_head);

static mbtcp_session_t _sessions[MBTCP_MAX_SESSIONS] = {0};
static uint8_t _data_buf[MBTCP_DATA_BUFSZ];

static void _session_recv_step_reset(mbtcp_session_t *session) {
    session->data_step = STEP_HEAD;
    session->data_len = 0;
    session->len_to_read = 4;
}

int mbtcp_all_sessions_set_fds(fd_set *readset, fd_set *exceptset) {
    int maxfd = 0;
    mbtcp_session_t *session;
    LIST_FOREACH(session, &_session_head, entries) {
        if (maxfd < session->socket + 1) maxfd = session->socket + 1;
        FD_SET(session->socket, readset);
        FD_SET(session->socket, exceptset);
    }

    return maxfd;
}

mbtcp_session_t *mbtcp_session_take(void) {
    mbtcp_session_t *session = NULL;

    for (int i = 0; i < MBTCP_MAX_SESSIONS; i++) {
        if (_sessions[i].active == 0) {
            session = &_sessions[i];
            break;
        }
    }

    if (session) {
        session->active = 1;
        session->socket = -1;
        memset(&session->cliaddr, 0, sizeof(session->cliaddr));

        LIST_INSERT_HEAD(&_session_head, session, entries);
    }

    return session;
}

int mbtcp_session_init(mbtcp_session_t *session, int socket, struct sockaddr_storage *addr, socklen_t clilen) {
    int option = 1;
    int flags = 0;

    session->socket = socket;
    memset(&session->cliaddr, 0, sizeof(session->cliaddr));
    memcpy(&session->cliaddr, addr, clilen);

    session->timeout = xTaskGetTickCount() + pdMS_TO_TICKS(MBTCP_SESSION_TIMEOUT);
    agile_modbus_tcp_init(&session->mb_ctx, session->send_buf, sizeof(session->send_buf), session->read_buf, sizeof(session->read_buf));
    agile_modbus_set_slave(&session->mb_ctx._ctx, 1);
    _session_recv_step_reset(session);
    session->recv_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(MBTCP_SESSION_RECV_TIMEOUT);

    if (setsockopt(session->socket, IPPROTO_TCP, TCP_NODELAY, (const void *)&option, sizeof(int)) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "socket %d setsockopt TCP_NODELAY failed", session->socket);
    }

    flags = fcntl(session->socket, F_GETFL);

    if (fcntl(session->socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "socket %d ioctlsocket FIONBIO failed", session->socket);
    }

    return 0;
}

void mbtcp_session_close(mbtcp_session_t *session) {
    LIST_REMOVE(session, entries);

    if (session->socket >= 0) {
        close(session->socket);
        session->socket = -1;
    }

    session->active = 0;
}

void mbtcp_session_close_all(void) {
    mbtcp_session_t *session, *session_next;
    LIST_FOREACH_SAFE(session, &_session_head, entries, session_next) {
        mbtcp_session_close(session);
    }
}

void mbtcp_session_check_timeout(void) {
    mbtcp_session_t *session, *session_next;

    LIST_FOREACH_SAFE(session, &_session_head, entries, session_next) {
        if ((xTaskGetTickCount() - session->timeout) < (portMAX_DELAY / 2)) {
            ESP_LOGW(LOG_LOCAL_TAG, "socket %d timeout, close it.", session->socket);
            mbtcp_session_close(session);
        }
    }
}

void mbtcp_session_check_recv_timeout(mbtcp_session_t *session) {
    if (session == NULL) {
        mbtcp_session_t *session;

        LIST_FOREACH(session, &_session_head, entries) {
            if ((xTaskGetTickCount() - session->recv_timeout) < (portMAX_DELAY / 2)) {
                _session_recv_step_reset(session);
                session->recv_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(MBTCP_SESSION_RECV_TIMEOUT);
            }
        }
    } else {
        if ((xTaskGetTickCount() - session->recv_timeout) < (portMAX_DELAY / 2)) {
            _session_recv_step_reset(session);
            session->recv_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(MBTCP_SESSION_RECV_TIMEOUT);
        }
    }
}

static int _protocol_preprocess(mbtcp_session_t *session, uint8_t *data, uint32_t data_len) {
    int rc = -1;

    if (data_len == 0) return 0;

    switch (session->data_step) {
        case STEP_HEAD: {
            if (data_len < session->len_to_read) {
                memcpy(session->read_buf + session->data_len, data, data_len);
                rc = data_len;
                session->data_len += data_len;
                session->len_to_read -= data_len;
            } else {
                memcpy(session->read_buf + session->data_len, data, session->len_to_read);
                rc = session->len_to_read;
                session->data_len += session->len_to_read;
                if (session->read_buf[2] != 0 || session->read_buf[3] != 0) {
                    rc = -1;
                    break;
                }
                session->len_to_read = 3;
                session->data_step = STEP_MBAP;
            }
        } break;

        case STEP_MBAP: {
            if (data_len < session->len_to_read) {
                memcpy(session->read_buf + session->data_len, data, data_len);
                rc = data_len;
                session->data_len += data_len;
                session->len_to_read -= data_len;
            } else {
                memcpy(session->read_buf + session->data_len, data, session->len_to_read);
                rc = session->len_to_read;
                session->data_len += session->len_to_read;
                session->len_to_read = ((session->read_buf[4] << 8) | session->read_buf[5]) - 1;
                if ((session->len_to_read + AGILE_MODBUS_TCP_HEADER_LENGTH) > sizeof(session->read_buf)) {
                    rc = -1;
                    break;
                }
                session->data_step = STEP_PDU;
            }
        } break;

        case STEP_PDU: {
            if (data_len < session->len_to_read) {
                memcpy(session->read_buf + session->data_len, data, data_len);
                rc = data_len;
                session->data_len += data_len;
                session->len_to_read -= data_len;
            } else {
                memcpy(session->read_buf + session->data_len, data, session->len_to_read);
                rc = session->len_to_read;
                session->data_len += session->len_to_read;
                session->len_to_read = 0;
                session->data_step = STEP_OK;
            }
        } break;

        case STEP_OK:
            break;

        default:
            break;
    }

    return rc;
}

static int _protocol_process(mbtcp_session_t *session, uint8_t *data, int sz) {
    uint8_t *ptr = data;
    agile_modbus_t *ctx = &session->mb_ctx._ctx;

    while (sz > 0) {
        int rc = _protocol_preprocess(session, ptr, sz);
        if (rc >= 0) {
            ptr += rc;
            sz -= rc;
        } else {
            _session_recv_step_reset(session);
            ptr++;
            sz--;
        }

        if (session->data_step == STEP_OK) {
            ESP_LOGI(LOG_LOCAL_TAG, "socket %d", session->socket);
            printf("---> ");
            for (int i = 0; i < session->data_len; i++) {
                printf("%02X ", session->read_buf[i]);
            }
            printf("\r\n\r\n");

            rc = agile_modbus_slave_handle(ctx, session->data_len, 0, agile_modbus_slave_util_callback, &mb_slave_util, NULL);
            if (rc > 0) {
                printf("<--- ");
                for (int i = 0; i < rc; i++) {
                    printf("%02X ", ctx->send_buf[i]);
                }
                printf("\r\n\r\n");

                if (send(session->socket, ctx->send_buf, rc, 0) != rc) {
                    ESP_LOGW(LOG_LOCAL_TAG, "socket %d send failed, close it.", session->socket);
                    return -1;
                }

                session->timeout = xTaskGetTickCount() + pdMS_TO_TICKS(MBTCP_SESSION_TIMEOUT);
            }

            _session_recv_step_reset(session);
        }
    }

    return 0;
}

static int _session_handle(mbtcp_session_t *session) {
    int rc = -1;

    rc = recv(session->socket, _data_buf, sizeof(_data_buf), MSG_DONTWAIT);
    if (rc <= 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "socket %d read failed, close it.", session->socket);
        return -1;
    }

    rc = _protocol_process(session, _data_buf, rc);
    if (rc < 0) {
        ESP_LOGW(LOG_LOCAL_TAG, "socket %d protocol process failed, close it.", session->socket);
        return -1;
    }

    session->recv_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(MBTCP_SESSION_RECV_TIMEOUT);

    return 0;
}

void mbtcp_session_handle(fd_set *readset, fd_set *exceptset) {
    mbtcp_session_t *session, *session_next;

    LIST_FOREACH_SAFE(session, &_session_head, entries, session_next) {
        // readset, exceptset
        if (FD_ISSET(session->socket, exceptset)) {
            ESP_LOGW(LOG_LOCAL_TAG, "socket %d except, close it.", session->socket);
            mbtcp_session_close(session);
        } else {
            if (FD_ISSET(session->socket, readset)) {
                if (_session_handle(session) < 0) {
                    mbtcp_session_close(session);
                }
            } else {
                mbtcp_session_check_recv_timeout(session);
            }
        }
    }
}
