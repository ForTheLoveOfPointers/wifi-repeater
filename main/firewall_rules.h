#pragma once

#include "esp_netif.h"
#include "lwip/raw.h"
#include "lwip/pbuf.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/icmp.h"
#include "lwip/ethip6.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/ip4.h"


/* Simple rule structure: match IP, proto, optional ports */
typedef enum { ACTION_ALLOW = 0, ACTION_DENY = 1 } fw_action_t;

typedef struct {
    ip4_addr_t src;      /* IP_ADDR_ANY for wildcard */
    ip4_addr_t dst;      /* IP_ADDR_ANY for wildcard */
    uint8_t proto;       /* IP protocol (IP_PROTO_TCP / UDP / ICMP) */
    uint16_t src_port;   /* 0 = any */
    uint16_t dst_port;   /* 0 = any */
    fw_action_t action;
} fw_rule_t;