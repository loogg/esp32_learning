#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "ping/ping_sock.h"
#include "shell.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define TAG             "netutils.ping"
#include "esp_log.h"

static void cmd_ping_on_ping_success(esp_ping_handle_t hdl, void *args) {
    uint8_t   ttl;
    uint16_t  seqno;
    uint32_t  elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI(TAG, "%" PRIu32 " bytes from %s icmp_seq=%" PRIu16 " ttl=%" PRIu16 " time=%" PRIu32 " ms", recv_len,
             ipaddr_ntoa((ip_addr_t *)&target_addr), seqno, ttl, elapsed_time);
}

static void cmd_ping_on_ping_timeout(esp_ping_handle_t hdl, void *args) {
    uint16_t  seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGI(TAG, "From %s icmp_seq=%d timeout", ipaddr_ntoa((ip_addr_t *)&target_addr), seqno);
}

static void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args) {
    ip_addr_t target_addr;
    uint32_t  transmitted;
    uint32_t  received;
    uint32_t  total_time_ms;
    uint32_t  loss;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));

    if (transmitted > 0) {
        loss = (uint32_t)((1 - ((float)received) / transmitted) * 100);
    } else {
        loss = 0;
    }
    if (IP_IS_V4(&target_addr)) {
#if CONFIG_LWIP_IPV4
        ESP_LOGI(TAG, "--- %s ping statistics ---", inet_ntoa(*ip_2_ip4(&target_addr)));
#endif
    } else {
#if CONFIG_LWIP_IPV6
        ESP_LOGI(TAG, "--- %s ping statistics ---", inet6_ntoa(*ip_2_ip6(&target_addr)));
#endif
    }
    ESP_LOGI(TAG, "%" PRIu32 " packets transmitted, %" PRIu32 " received, %" PRIu32 "%% packet loss, time %" PRIu32 "ms", transmitted, received, loss,
             total_time_ms);
    // delete the ping sessions, so that we clean up all resources and can create a new ping session
    esp_ping_delete_session(hdl);
}

static int cmd_ping(int argc, char *argv[]) {
    if (argc < 2) {
        ESP_LOGE(TAG, "Usage: ping <host>");
        return -1;
    }

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();

    // parse IP address
    struct sockaddr_in6 sock_addr6;
    ip_addr_t           target_addr = {0};

    if (inet_pton(AF_INET6, argv[1], &sock_addr6.sin6_addr) == 1) {
        /* convert ip6 string to ip6 address */
        ipaddr_aton(argv[1], &target_addr);
    } else {
        struct addrinfo  hint = {0};
        struct addrinfo *res  = NULL;

        /* convert ip4 string or hostname to ip4 or ip6 address */
        if (getaddrinfo(argv[1], NULL, &hint, &res) != 0) {
            ESP_LOGE(TAG, "ping: unknown host %s", argv[1]);
            return -1;
        }
        if (res->ai_family == AF_INET) {
#if CONFIG_LWIP_IPV4
            struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
            inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
#endif
        } else {
#if CONFIG_LWIP_IPV6
            struct in6_addr addr6 = ((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr;
            inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
#endif
        }
        freeaddrinfo(res);
    }
    config.target_addr = target_addr;

    /* set callback functions */
    esp_ping_callbacks_t cbs = {.cb_args         = NULL,
                                .on_ping_success = cmd_ping_on_ping_success,
                                .on_ping_timeout = cmd_ping_on_ping_timeout,
                                .on_ping_end     = cmd_ping_on_ping_end};
    esp_ping_handle_t    ping;
    esp_ping_new_session(&config, &cbs, &ping);
    esp_ping_start(ping);

    return 0;
}
SHELL_EXPORT_CMD(ping, cmd_ping, Send ICMP ECHO_REQUEST to network hosts.);
