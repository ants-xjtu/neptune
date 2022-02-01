/* Modified by Qcloud1223 
 * 1. change from dynamic linking to runtime linking
 * 2. suppress printf
*/

#define _GNU_SOURCE
#include <libnet.h>
#include <malloc.h>
#include <nids.h>
#include <pcap.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>

#include "../PrivateHeap.h"
#include "../PrivateStack.h"

char ascii_string[10000];
int state[7] = {0};          // a simple statistics on libnids
static int dirty_pages;
static int pkt_cnt;
static int start;

char *char_to_ascii(char ch) /* 用于把协议数据进行显示 */
{
    memset(ascii_string, 0x00, 10000);
    ascii_string[0] = 0;
    char *string = ascii_string;
    if (isgraph(ch))
    /* 可打印字符 */
    {
        *string++ = ch;
    }
    else if (ch == ' ')
    /* 空格 */
    {
        *string++ = ch;
    }
    else if (ch == '\n' || ch == '\r')
    /* 回车和换行 */
    {
        *string++ = ch;
    }
    else
    /* 其它字符以点"."表示 */
    {
        *string++ = '.';
    }
    *string = 0;
    return ascii_string;
}
/*
 * ====================================================================================
 * 下面的函数是回调函数，用于分析TCP连接，分析TCP连接状态，对TCP协议传输的数据进行分析
 * ====================================================================================
 *  
 */
void tcp_protocol_callback(struct tcp_stream *tcp_connection, void **arg)
{
    // printf("tcp_protocol_callback\n");
    int i;
    char address_string[1024];
    static char content[65535];
    // char content_urgent[65535];
    struct tuple4 ip_and_port = tcp_connection->addr;
    /* 获取TCP连接的地址和端口对 */
    strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
    /* 获取源地址 */
    sprintf(address_string + strlen(address_string), " : %i", ip_and_port.source);
    /* 获取源端口 */
    strcat(address_string, " <---> ");
    strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
    /* 获取目的地址 */
    sprintf(address_string + strlen(address_string), " : %i", ip_and_port.dest);
    /* 获取目的端口 */
    strcat(address_string, "\n");
    switch (tcp_connection->nids_state) /* 判断LIBNIDS的状态 */
    {
    case NIDS_JUST_EST:
        /* 表示TCP客户端和TCP服务器端建立连接状态 */

        /* 在此状态下就可以决定是否对此TCP连接进行数据分析，可以决定是否捕获TCP客户端接收的数据、TCP服务器端接收的数据、TCP客户端接收的紧急数据或者TCP服务器端接收的紧急数据*/
        tcp_connection->client.collect++;
        /* 客户端接收数据 */
        tcp_connection->server.collect++;
        /* 服务器接收数据 */
        tcp_connection->server.collect_urg++;
        /* 服务器接收紧急数据 */
        tcp_connection->client.collect_urg++;
        /* 客户端接收紧急数据 */
        // printf("%sTCP连接建立\n", address_string);
        state[NIDS_JUST_EST]++;
        return;
    case NIDS_CLOSE:
        /* 表示TCP连接正常关闭 */
        // printf("--------------------------------\n");
        // printf("%sTCP连接正常关闭\n", address_string);
        state[NIDS_CLOSE]++;
        return;
    case NIDS_RESET:
        /* 表示TCP连接被重置关闭 */
        // printf("--------------------------------\n");
        // printf("%sTCP连接被RST关闭\n", address_string);
        state[NIDS_RESET]++;
        return;
    case NIDS_DATA:
        // 表示有新的数据到达
        //在这个状态可以判断是否有新的数据到达，如果有就可以把数据存储起来，可以在这个状态之中来分析TCP传输的数据，此数据就存储在half_stream数据结构的缓存之中
        {
            state[NIDS_DATA]++;
            struct half_stream *hlf;
            /* 表示TCP连接的一端的信息，可以是客户端，也可以是服务器端 */
            if (tcp_connection->server.count_new_urg)
            {
                /* 表示TCP服务器端接收到新的紧急数据 */
                // printf("--------------------------------\n");
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                sprintf(address_string + strlen(address_string), " : %i", ip_and_port.source);
                strcat(address_string, " urgent---> ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                sprintf(address_string + strlen(address_string), " : %i", ip_and_port.dest);
                strcat(address_string, "\n");
                address_string[strlen(address_string) + 1] = 0;
                address_string[strlen(address_string)] = tcp_connection->server.urgdata;
                // printf("%s", address_string);
                return;
            }
            if (tcp_connection->client.count_new_urg)
            {
                /* 表示TCP客户端接收到新的紧急数据 */
                // printf("--------------------------------\n");
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                sprintf(address_string + strlen(address_string), " : %i", ip_and_port.source);
                strcat(address_string, " <--- urgent ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                sprintf(address_string + strlen(address_string), " : %i", ip_and_port.dest);
                strcat(address_string, "\n");
                address_string[strlen(address_string) + 1] = 0;
                address_string[strlen(address_string)] = tcp_connection->client.urgdata;
                // printf("%s", address_string);
                return;
            }
            if (tcp_connection->client.count_new)
            {
                /* 表示客户端接收到新的数据 */
                hlf = &tcp_connection->client;
                /* 此时hlf表示的是客户端的TCP连接信息 */
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                sprintf(address_string + strlen(address_string), ":%i", ip_and_port.source);
                strcat(address_string, " <--- ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                sprintf(address_string + strlen(address_string), ":%i", ip_and_port.dest);
                strcat(address_string, "\n");
                // printf("--------------------------------\n");
                // printf("%s", address_string);
                memcpy(content, hlf->data, hlf->count_new);
                content[hlf->count_new] = '\0';
                // printf("客户端接收数据\n");
                //for (i = 0; i < hlf->count_new; i++) {
                //    printf("%s", char_to_ascii(content[i]));
                //    // 输出客户端接收的新的数据，以可打印字符进行显示
                //}
                // printf("\n");
                // return;
            }
            else
            {
                /* 表示服务器端接收到新的数据 */
                hlf = &tcp_connection->server;
                /* 此时hlf表示服务器端的TCP连接信息 */
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                sprintf(address_string + strlen(address_string), ":%i", ip_and_port.source);
                strcat(address_string, " ---> ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                sprintf(address_string + strlen(address_string), ":%i", ip_and_port.dest);
                strcat(address_string, "\n");
                // printf("--------------------------------\n");
                // printf("%s", address_string);
                memcpy(content, hlf->data, hlf->count_new);
                content[hlf->count_new] = '\0';
                // printf("服务器端接收数据:\n");
                //for (i = 0; i < hlf->count_new; i++) {
                //    printf("%s", char_to_ascii(content[i]));
                // 输出服务器接收到的新的数据
                //}
                // printf("\n");
                // return;
            }
            // for (int i = 10; i < 20; i += 1) {
            //     memcpy(&content[i * 1024], &content[(i - 10) * 1024], 1024);
            // }
            return;
        }
    default:
        state[0]++;
        break;
    }
    return;
}

// don't mprotect until all calls to libc ends
struct 
{
    void *start;
    uint64_t len;
    int prot;
    // sometimes we want to ignore this, e.g. stack
    int force;
} to_mprotect[64];

// use SIGINT as a signal to start page monitoring
void page_monitor(int signum, siginfo_t *info, void *context)
{
    static int first = 1;
    static struct timespec ts, te;
    if (first)
    {
        printf("Signal %d received, start to change permission\n", signum);
        // if this is the first time ctrl-C is pressed, mprotect all its pages to read-only
        FILE *map = fopen("/proc/self/maps", "r");
        if (map == NULL)
        {
            fprintf(stderr, "[page_monitor] cannot open map file\n");
            return;
        }
        char lineBuf[256];
        uint64_t addrStart, addrEnd = 0, prevEnd = 0;
        char perm[5];
        uint32_t offset;
        uint16_t devHigh, devLow;
        unsigned int inode;
        char pathname[128];
        int nConsumed;
        uint64_t mapLength;
        int pagecnt = 0;
        int anon;
        while (fgets(lineBuf, sizeof(lineBuf), map))
        {
            anon = 0;
            sscanf(lineBuf, "%lx-%lx %4c %x %hx:%hx %u%n", &addrStart, &addrEnd, perm, &offset, &devHigh, &devLow, &inode, &nConsumed);
            mapLength = addrEnd - addrStart;
            // don't mess with the shared mappings
            if (perm[3] == 's')
                continue;
            // pathname[0] = '\0';
            // if (devHigh || devLow || inode)
            // {
            //     sscanf(lineBuf+nConsumed, "%s", pathname);
            //     // printf("0x%lx-0x%lx %s\n", addrStart, addrEnd, pathname);
            // }
            sscanf(lineBuf+nConsumed, "%s", pathname);
            if (*(lineBuf + nConsumed) == '\n')
                anon = 1;
            if (perm[1] == 'w')
            {
                // writeable implies readable
                int pr = 0;
                pr |= (perm[2] == 'x')? PROT_EXEC: 0;
                pr |= PROT_READ;
                to_mprotect[pagecnt].start = (void *)addrStart;
                to_mprotect[pagecnt].len = mapLength;
                to_mprotect[pagecnt++].prot = pr;
                if (strcmp(pathname, "[stack]") == 0 && anon == 0)
                    to_mprotect[pagecnt - 1].force = 1;
                // if (strcmp(pathname, "/usr/lib/x86_64-linux-gnu/libc-2.31.so") == 0)
                //     to_mprotect[pagecnt - 1].force = 1;
                if ((uint64_t)&dirty_pages >= addrStart && (uint64_t)&dirty_pages <= addrEnd)
                    to_mprotect[pagecnt - 1].force = 1;
                // if (mprotect((void *)addrStart, mapLength, pr | PROT_READ))
                // {
                //     fprintf(stderr, "[page_monitor] mprotect failed with: addr=%p, len=%lu, prot=%d", (void *)addrStart, mapLength, pr);
                //     return;
                // }
            }
        }
        first = 0;
        // when all mprotects are done, record time and wait for next time the handler is called
        clock_gettime(CLOCK_REALTIME, &ts);
        for (int i = 0; i<64; i++)
        {
            if (to_mprotect[i].start == NULL)
                break;
            if (to_mprotect[i].force == 1)
                continue;
            if (mprotect(to_mprotect[i].start, to_mprotect[i].len, to_mprotect[i].prot))
            {
                fprintf(stderr, "[page_monitor] mprotect failed with: addr=%p, len=%lu, prot=%d", 
                    to_mprotect[i].start, to_mprotect[i].len, to_mprotect[i].prot);
                return;
            }
        }
        start = 1;
        // since heap is read only, using libc functions would likely fail.
        // printf("Change permission done, please check layout\n");
        // int sleeper;
        // scanf("%d", &sleeper);
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &te);
        printf("Signal %d received(2nd), print statistics and quit\n", signum);
        printf("Time elapsed: %f, dirty pages: %d\n", (double)(te.tv_nsec - ts.tv_nsec) / 1e9 + (te.tv_sec - ts.tv_sec), dirty_pages);
        // game_over();
    }
}

void segv_handler(int signum, siginfo_t *info, void *context)
{
    size_t addr_num = ((size_t)info->si_addr >> 12) << 12;
    // printf("Instruction pointer: %p\n",
    //        (((ucontext_t*)context)->uc_mcontext.gregs[16]));
    // printf("Addr: %p\n", (void *)addr_num);
    // assume no page would have 'w' and 'x' at the same time
    if (mprotect((void *)addr_num, 4096, PROT_READ | PROT_WRITE))
    {
        printf("restoring memory protection failed\n");
        abort();
    }
    dirty_pages++;
}

void nids_run_worker()
{
    struct timespec ts = {0, 0}, te = {0, 0};
    nids_run();
    /* Libnids进入循环捕获数据包状态 */
    clock_gettime(CLOCK_REALTIME, &te);
    // I really have no idea why this gives a SIGFPE...
    // double t = ((double)te.tv_sec + 1.0e-9 * te.tv_nsec) -
    //           ((double)ts.tv_sec + 1.0e-9 * ts.tv_nsec);
    int sec_val = te.tv_sec - ts.tv_sec;
    // int nsec_val = sec_val * 1000000000;
    // int real_time = nsec_val + te.tv_nsec - ts.tv_nsec;
    printf("Time elapsed: %d seconds, %ld nanoseconds\n", sec_val, te.tv_nsec - ts.tv_nsec);
}

// NB: if SA_SIGINFO is not set, sigaction takes a function with void (*)(int)
void sigterm_handler(int signum)
{
    printf("Sigterm received, exit.\n");
    printf("final dirty page count: %d\n", dirty_pages);
    exit(0);
}

int main(int argc, char *argv[], char **env)
{
    struct nids_chksum_ctl temp;
    temp.netaddr = 0;
    temp.mask = 0;
    temp.action = 1;

    nids_register_chksum_ctl(&temp, 1);
    /*这段是相关与计算校验和的，比较新的网卡驱动会自动计算校验和，我们要做的就是把它关掉*/
    nids_params.filename = "/home/hypermoon/enterprise.pcap";
    // nids_params.device = "all";
    struct sigaction act1, act2, act3;
    act1.sa_flags = SA_SIGINFO | SA_NODEFER;
    act1.sa_sigaction = &page_monitor;
    act2.sa_flags = SA_SIGINFO | SA_NODEFER;
    act2.sa_sigaction = &segv_handler;
    sigaction(SIGINT, &act1, NULL);
    sigaction(SIGSEGV, &act2, NULL);
    act3.sa_flags = 0;
    act3.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &act3, NULL);
    if (!nids_init()) /* Libnids初始化 */
    {
        printf("Error：%s\n", nids_errbuf);
        exit(1);
    }
    nids_register_tcp((void *)tcp_protocol_callback);
    // former wrapper is no longer needed
    // nids_run_worker();

    // do nids_run multiply times, hopefully this would make libpcap read the capture many times
    nids_run();
    // nids_init();
    // nids_run();
    // nids_init();
    // nids_run();
    // nids_init();
    // nids_run();
    // nids_init();
    // nids_run();
    printf("libnids reach the end of the pcap and return\n");

    return 0;
}
