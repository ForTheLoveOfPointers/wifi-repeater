#include "firewall_rules.h"


/** 
 * ATTENTION:
 * Many implemented rules are very simple and you will have trouble using this for anything other than testing 
 * and building, on top of it, your own implementation. Parsing can technically be done to some extent, but the ESP32 
 * is not the best option. If you ever need to parse data to implement some rules regarding, for example, certain content,
 * you can always use the pbuf_strstr() and pbuf_dechain(), for example, to diminish overhead.
 * 
*/
static fw_rule_t rules[] = {
    {.src = IP_ADDR_ANY, .dst = IP_ADDR_ANY, .proto = IP_PROTO_TCP, .src_port = 0, .dst_port = 23, .action = ACTION_DENY}
};

// This makes the header readable, switching bytes [0] and [1]
// No use of ntohs because bytes might not be aligned
static uint16_t read_u16_be(void* ptr) {
    const uint8_t *b = (const uint8_t*)ptr;
    return (uint16_t)( (b[0] << 8) | b[1]); 
}

bool ip4_is_any(ip4_addr_t *addr) {
    return addr->addr == 0;
}


static bool firewall_match_and_deny(struct pbuf *p) {
    struct ip_hdr *iphdr = (struct ip_hdr *)p->payload;
    if(IPH_V(ip_hdr) != 4) return false; // If not IPv4, we don't match rules

    uint8_t proto = IPH_PROTO(ip_hdr);
    
    ip4_addr_t src, dst; 
    ip4_addr_set_u32(&src, iphdr->src.addr); // This function is the officially provided one by lwIP. DO NOT CHANGE
    ip4_addr_set_u32(&dst, iphdr->dest.addr);

    int ihl = IPH_HL(iphdr) * 4;
    // Check if src_port and dst_port are inside the first pbuffer of the chain
    // If not, then it is better not to continue, for now
    if (ihl + 4 <= p->len) {
        /* if TCP/UDP, attempt to read ports from payload */
        if (proto == IP_PROTO_TCP) {
            /* transport header begins after IP header */
            uint8_t *transport = (uint8_t *)((uint8_t*)p->payload + ihl);
            /* p->len might be smaller than needed if pbuf is chained; for production code,
               use pbuf_copy_partial to safely read across chained pbufs. */
            src_port = read_u16_be(transport);
            dst_port = read_u16_be(transport + 2);
        }
    } else {
        /* header truncated - be conservative and not drop */
        return false;
    }

     /* match rules in order; first match wins */
    for (size_t i = 0; i < rules_count; ++i) {
        fw_rule_t *r = &rules[i];

        if (r->proto && r->proto != proto) continue;

        if (!ip4_is_any(&r->src) && r->src.addr != src.addr) continue;
        if (!ip4_is_any(&r->dst) && r->dst.addr != dst.addr) continue;

        if (r->src_port && r->src_port != src_port) continue;
        if (r->dst_port && r->dst_port != dst_port) continue;

        if (r->action == ACTION_DENY) {
            ESP_LOGI(TAG, "DENY matched rule %u proto=%u s=%s:%u d=%s:%u",
                     (unsigned)i, proto,
                     ip4addr_ntoa(&src), ntohs(src_port),
                     ip4addr_ntoa(&dst), ntohs(dst_port));
            return true; /* deny => caller should free pbuf and eat packet */
        } else {
            ESP_LOGI(TAG, "ALLOW matched rule %u", (unsigned)i);
            return false;
        }
    }

    /* no rule matched -> default ALLOW (you could flip default) */
    return false;

}
