#include "firewall_rules.h"


/** 
 * ATTENTION:
 * Many implemented rules are very simple and you will have trouble using this for anything other than testing 
 * and building, on top of it, your own implementation. Parsing can technically be done to some extent, but the ESP32 
 * is not the best option. If you ever need to parse data to implement some rules regarding, for example, certain content,
 * you can always use the pbuf_strstr() and pbuf_dechain(), for example, to diminish overhead.
 * 
 * Firewall rules are stored in nvs, inside the firewall_cfg_nvs key, as a JSON string. The JSON structure is like this:
 * 
 * {
 *      config: [
 *          {src: 0.0.0.0, dst: 0.0.0.0, proto: 0, src_port: 0, dst_port: 23, action: 1},
 *                                      ...
 *                                      ...
 *      ]
 * }
 * 
 * When reading src and dst, you need the aton conversion done, as, to store them as str, you first have to go ntoa
 * 
*/
static fw_rule_t rules[5];

const static char *TAG = "fw_rules_mod";
const static char *firewall_partition_nvs =  "firewall_rules";
const static char *firewall_cfg_nvs =  "config";

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
    
    uint16_t src_port = 0, dst_port = 0;
    
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


/**
 * PCBs (Protocol Control Blocks)
 */

 //Necessary callback for the raw_recv function. Processes every packet buffer
static u8_t fw_raw_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(addr);

    /* If we want to eat/drop the packet: free p and return non-zero */
    if (firewall_match_and_deny(p)) {
        pbuf_free(p); /* tell lwIP we've consumed/dropped it */
        return 1; /* eaten */
    }
    /* not matched -> return 0 to let lwIP continue processing */
    return 0;
}

/**
 * Create raw pcbs for protocols we want to control 
 * Commented udp and icmp to allow later, as tcp will be a testing protocol
 */
static struct raw_pcb *pcb_tcp = NULL;
static struct raw_pcb *pcb_udp = NULL;
static struct raw_pcb *pcb_icmp = NULL;

static void firewall_create_pcbs(void *arg) {
    LWIP_UNUSED_ARG(arg);

    pcb_tcp = raw_new(IP_PROTO_TCP);
    if (pcb_tcp) raw_recv(pcb_tcp, fw_raw_recv, NULL);

    pcb_udp = raw_new(IP_PROTO_UDP);
    if (pcb_udp) raw_recv(pcb_udp, fw_raw_recv, NULL);

    pcb_icmp = raw_new(IP_PROTO_ICMP);
    if (pcb_icmp) raw_recv(pcb_icmp, fw_raw_recv, NULL);

    ESP_LOGI(TAG, "firewall pcbs created");
}

esp_err_t load_rules(nvs_handle_t nvs, char* buf) {
    if(nvs_open(firewall_partition_nvs, NVS_READONLY, nvs) != ESP_OK) {
        return ESP_FAIL;
    };
    int size = 0;
    nvs_get_str(nvs, firewall_cfg_nvs, NULL, &size); // This will retrieve the size of the stored str
    buf = (char*)malloc(size);
    nvs_get_str(nvs, firewall_cfg_nvs, buf, &size);
    nvs_close(nvs);
    return ESP_OK;
}

static void make_fw_rules_arr(char *buf) {
    cJSON *root = cJSON_parse(buf);
    free(buf);
    cJSON *arr = cJSON_GetObjectItem(root, firewall_cfg_nvs);
    int size = cJSON_GetArraySize(arr);
    for(int i = 0; i < size; i++) {
        // Do the JSON loading into the firewall_arr
    }
}

/* call during init — Not static, and runs in tcpip thread via tcpip_callback */
void firewall_init(void) {
    nvs_handle_t nvs;
    
    char *fw_rls = NULL;
    if(load_rules(nvs, fw_rls) != ESP_OK) {
        ESP_LOGE(TAG, "could not load firewall rules");
    } else {
        make_fw_rules_arr(fw_rls);
    }
    

    /* tcpip_callback runs function on TCP/IP thread. ERR_OK comes from the lwIP specification */
    if (tcpip_callback(firewall_create_pcbs, NULL) != ERR_OK) {
        ESP_LOGE(TAG, "tcpip_callback failed");
    }
}

/* cleanup if needed (remove raw pcbs) */
static void firewall_remove_pcbs(void *arg) {
    if (pcb_tcp) { raw_remove(pcb_tcp); pcb_tcp = NULL; }
    if (pcb_udp) { raw_remove(pcb_udp); pcb_udp = NULL; }
    if (pcb_icmp){ raw_remove(pcb_icmp); pcb_icmp = NULL; }
}