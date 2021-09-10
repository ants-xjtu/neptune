#include "Common.h"

struct flow_rule {
    struct rte_flow* flow_handle;

    struct rte_flow_attr attr;
    struct rte_flow_item pattern[2];
    struct rte_flow_action action[2];

    char flow_id;
    unsigned queue_id;
    unsigned port_id;
    struct rte_flow_item_eth eth_spec;
    struct rte_flow_item_eth eth_mask;
    struct rte_flow_action_queue queue;
    struct rte_flow_action queue_action;
};

static void dump_rules(struct flow_rule** rules, int nb_rules)
{
    for (int i=0; i<nb_rules; i++) {
        printf("flow id: %u, queue id: %u\n", rules[i]->flow_id, rules[i]->queue_id);
    } 
}

static int create_flow_rules(struct flow_rule** flow_rules, int flow_queue_map[][2], int nb_rules)
{
    // flow_rules = (struct flow_rule**)malloc(nb_rules * sizeof(struct flow_rule*));
    printf("\nready to create %d flows\n", nb_rules);

    u_char mac_base[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    u_char mac_empty[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    u_char mask_base[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xff};

    for (int i=0; i<nb_rules; i++) {
        printf("creating %dth flow\n", i);
        flow_rules[i] = (struct flow_rule*)malloc(sizeof(struct flow_rule));
        struct flow_rule* rule = flow_rules[i];
        rule->port_id = 0;

        memset(&rule->eth_spec, 0, sizeof(struct rte_flow_item_eth));
        memset(&rule->eth_mask, 0, sizeof(struct rte_flow_item_eth));
    
        /* only check the last byte of the dst mac */
        memcpy(&rule->eth_mask.hdr.d_addr, mask_base, 6);
        memcpy(&rule->eth_mask.hdr.s_addr, mac_empty, 6);
        rule->eth_mask.hdr.ether_type = 0x0;

        u_char flow_id = flow_queue_map[i][0];
        printf("flow id: %u\n", flow_id);
        u_char dst_mac[6];
        memcpy(dst_mac, mac_base, 6);
        dst_mac[5] = flow_id;
        memcpy(&rule->eth_spec.hdr.d_addr, dst_mac, 6);
        rule->flow_id = flow_id;

        unsigned queue_id = flow_queue_map[i][1];
        printf("queue id: %u\n", queue_id);
        rule->queue_id = queue_id;
        rule->queue.index = queue_id;
        rule->queue_action.type = RTE_FLOW_ACTION_TYPE_QUEUE;
        rule->queue_action.conf = &rule->queue;

        memset(rule->pattern, 0, sizeof(rule->pattern));
        memset(rule->action, 0, sizeof(rule->action));
        memset(&rule->attr, 0, sizeof(struct rte_flow_attr));
        rule->attr.ingress = 1;

        rule->action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
        rule->action[0].conf = &rule->queue;
        rule->action[1].type = RTE_FLOW_ACTION_TYPE_END;

        rule->pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
        rule->pattern[0].spec = &rule->eth_spec;
        rule->pattern[0].mask = &rule->eth_mask;

        rule->pattern[1].type = RTE_FLOW_ITEM_TYPE_END;
    }

    dump_rules(flow_rules, nb_rules);
    return 0;
}

static int install_flow_rules(struct flow_rule** rules, int nb_rules)
{
    for (int i=0; i<nb_rules; i++) {
        struct flow_rule* rule = rules[i];
        struct rte_flow* flow = NULL;
        int res;
        struct rte_flow_error error[100];
        printf("ready to install %dth rule\n", i);
        res = rte_flow_validate(rule->port_id, &rule->attr, rule->pattern, rule->action, error);
        if (!res) {
            flow = rte_flow_create(rule->port_id, &rule->attr, rule->pattern, rule->action, error);
            rule->flow_handle = flow;
        }
        else {
            printf("invalidated flow when install flow\n");
            exit(-1);
        }
    }
    dump_rules(rules, nb_rules);
    return 0;
}

static int remove_flow_rules(struct flow_rule** rules, int nb_rules) 
{
    for (int i = 0; i < nb_rules; i++) {
        struct flow_rule* rule = rules[i];
        int res;
        struct rte_flow_error error[100];
        res = rte_flow_destroy(rule->port_id, rule->flow_handle, error);
    }
    return 0;
}

static int update_flow_rules(struct flow_rule** init, struct flow_rule** target, int nb_rules) 
{
    struct flow_rule* update_rules[1000];
    struct flow_rule* remove_rules[1000];
    int nb_updates = 0;

    for (int i=0; i<nb_rules; i++) {
        assert (init[i]->flow_id == target[i]->flow_id);
        if (init[i]->queue_id == target[i]->queue_id) {
            continue;
        }
        update_rules[nb_updates] = target[i];
        remove_rules[nb_updates] = init[i];
        nb_updates++;
    }
    remove_flow_rules(remove_rules, nb_updates);
    install_flow_rules(update_rules, nb_updates);
}

// utils finish
#define RULE_NUM 10
/* {flow_id, queue_id} */
// static int init_flow_queue_map[][2]   = {{1, 0}, {2, 0}, {3, 0}, {4, 1}, {5, 1}};
// static int target_flow_queue_map[][2] = {{1, 0}, {2, 0}, {3, 0}, {4, 1}, {5, 0}};
static int init_flow_queue_map[][2]   = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}};
static int target_flow_queue_map[][2] = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 1}, {8, 1}, {9, 1}};
static struct flow_rule *init_flow_rules[RULE_NUM];
static struct flow_rule *target_flow_rules[RULE_NUM];

void PrepareRules()
{
    create_flow_rules(init_flow_rules, init_flow_queue_map, RULE_NUM);
    create_flow_rules(target_flow_rules, target_flow_queue_map, RULE_NUM);
    dump_rules(init_flow_rules, RULE_NUM);
    // populate the rules to NIC
    install_flow_rules(init_flow_rules, RULE_NUM);
}

void UpdateRules()
{
    update_flow_rules(init_flow_rules, target_flow_rules, RULE_NUM);
}
