#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "shell.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#define TAG "netutils.ifconfig"
#include "esp_log.h"

static void print_iface_details(esp_netif_t *esp_netif)
{
    esp_netif_ip_info_t ip_info;
    uint8_t mac[NETIF_MAX_HWADDR_LEN];
    char interface[10];
#if CONFIG_LWIP_IPV6
    int ip6_addrs_count = 0;
    esp_ip6_addr_t ip6[LWIP_IPV6_NUM_ADDRESSES];
#endif
    esp_err_t ret = ESP_FAIL;
    esp_netif_dhcp_status_t status;

    struct netif *lwip_netif = esp_netif_get_netif_impl(esp_netif);

    /* Print Interface Name and Number */
    ret = esp_netif_get_netif_impl_name(esp_netif, interface);
    if ((ESP_FAIL == ret) || (NULL == esp_netif)) {
        ESP_LOGE(TAG, "No interface available");
        return;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
    if (esp_netif_get_default_netif() == esp_netif) {
        ESP_LOGI(TAG, "Interface Name: %s (DEF)", interface);
    } else {
        ESP_LOGI(TAG, "Interface Name: %s", interface);
    }
#else
    ESP_LOGI(TAG, "Interface Name: %s", interface);
#endif
    if (lwip_netif != NULL) {
        ESP_LOGI(TAG, "Interface Number: %d", lwip_netif->num);
    }

    ESP_LOGI(TAG, "Interface Key: %s", esp_netif_get_ifkey(esp_netif));
    ESP_LOGI(TAG, "Interface Desc: %s", esp_netif_get_desc(esp_netif));

    /* Print MAC address */
    esp_netif_get_mac(esp_netif, mac);
    ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
             mac[2], mac[3], mac[4], mac[5]);

    /* Print DHCP status */
    if (ESP_OK == esp_netif_dhcps_get_status(esp_netif, &status)) {
        ESP_LOGI(TAG, "DHCP Server Status: %s", (status == ESP_NETIF_DHCP_STARTED) || (status == ESP_NETIF_DHCP_STOPPED) ? "enabled" : "disabled");
    } else if ((ESP_OK == esp_netif_dhcpc_get_status(esp_netif, &status))) {
        if (ESP_NETIF_DHCP_STOPPED == status) {
            ESP_LOGI(TAG, "Static IP");
        } else {
            ESP_LOGI(TAG, "DHCP Client Status: %s", status ? "enabled" : "disabled");
        }
    }

    /* Print IP Info */
    esp_netif_get_ip_info(esp_netif, &ip_info);
    ESP_LOGI(TAG, "IP: " IPSTR ", MASK: " IPSTR ", GW: " IPSTR, IP2STR(&(ip_info.ip)), IP2STR(&(ip_info.netmask)), IP2STR(&(ip_info.gw)));

#if IP_NAPT
    /* Print NAPT status*/
    if (lwip_netif != NULL) {
        ESP_LOGI(TAG, "NAPT: %s", lwip_netif->napt ? "enabled" : "disabled");
    }
#endif

#if CONFIG_LWIP_IPV6
    /* Print IPv6 Address */
    ip6_addrs_count = esp_netif_get_all_ip6(esp_netif, ip6);
    for (int j = 0; j < ip6_addrs_count; ++j) {
        ESP_LOGI(TAG, "IPv6 address: " IPV6STR, IPV62STR(ip6[j]));
    }
#endif

    /* Print Interface and Link Status*/
    ESP_LOGI(TAG, "Interface Status: %s", esp_netif_is_netif_up(esp_netif) ? "UP" : "DOWN");
    if (lwip_netif != NULL) {
        ESP_LOGI(TAG, "Link Status: %s", netif_is_link_up(lwip_netif) ? "UP" : "DOWN");
    }
    ESP_LOGI(TAG, "");
}

static esp_err_t print_all_iface_details(void *ctx) {
    esp_netif_t *netif = NULL;

    while ((netif = esp_netif_next_unsafe(netif)) != NULL) {
        print_iface_details(netif);
    }

    return ESP_OK;
}

static int cmd_ifconfig(int argc, char *argv[]) {
    if (argc > 2) {
        ESP_LOGE(TAG, "Usage: ifconfig [if_key]");
        return -1;
    }

    if (argc == 1) {
        // 会影响协议栈性能，这里要确保网卡链表是安全的
        // esp_netif_tcpip_exec(print_all_iface_details, NULL);
        print_all_iface_details(NULL);
    } else {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey(argv[1]);
        if (netif == NULL) {
            ESP_LOGE(TAG, "No interface found with key: %s", argv[1]);
            return -1;
        }

        print_iface_details(netif);
    }

    return 0;
}
SHELL_EXPORT_CMD(ifconfig, cmd_ifconfig, Show the network interfaces configuration.);
