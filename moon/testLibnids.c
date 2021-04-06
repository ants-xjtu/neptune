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

#include "../PrivateHeap.h"
#include "../PrivateStack.h"

char ascii_string[10000];
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
    int i;
    char address_string[1024];
    char content[65535];
    // char content_urgent[65535];
    struct tuple4 ip_and_port = tcp_connection->addr;
    /* 获取TCP连接的地址和端口对 */
    strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
    /* 获取源地址 */
    //sprintf(address_string + strlen(address_string), " : %i", ip_and_port.source);
    /* 获取源端口 */
    strcat(address_string, " <---> ");
    strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
    /* 获取目的地址 */
    //sprintf(address_string + strlen(address_string), " : %i", ip_and_port.dest);
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
        //printf("%sTCP连接建立\n", address_string);
        return;
    case NIDS_CLOSE:
        /* 表示TCP连接正常关闭 */
        //printf("--------------------------------\n");
        //printf("%sTCP连接正常关闭\n", address_string);
        return;
    case NIDS_RESET:
        /* 表示TCP连接被重置关闭 */
        //printf("--------------------------------\n");
        //printf("%sTCP连接被RST关闭\n", address_string);
        return;
    case NIDS_DATA:
        // 表示有新的数据到达
        //在这个状态可以判断是否有新的数据到达，如果有就可以把数据存储起来，可以在这个状态之中来分析TCP传输的数据，此数据就存储在half_stream数据结构的缓存之中
        {
            struct half_stream *hlf;
            /* 表示TCP连接的一端的信息，可以是客户端，也可以是服务器端 */
            if (tcp_connection->server.count_new_urg)
            {
                /* 表示TCP服务器端接收到新的紧急数据 */
                //printf("--------------------------------\n");
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                //sprintf(address_string + strlen(address_string), " : %i", ip_and_port.source);
                strcat(address_string, " urgent---> ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                //sprintf(address_string + strlen(address_string), " : %i", ip_and_port.dest);
                strcat(address_string, "\n");
                address_string[strlen(address_string) + 1] = 0;
                address_string[strlen(address_string)] = tcp_connection->server.urgdata;
                //printf("%s", address_string);
                return;
            }
            if (tcp_connection->client.count_new_urg)
            {
                /* 表示TCP客户端接收到新的紧急数据 */
                //printf("--------------------------------\n");
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                //sprintf(address_string + strlen(address_string), " : %i", ip_and_port.source);
                strcat(address_string, " <--- urgent ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                //sprintf(address_string + strlen(address_string), " : %i", ip_and_port.dest);
                strcat(address_string, "\n");
                address_string[strlen(address_string) + 1] = 0;
                address_string[strlen(address_string)] = tcp_connection->client.urgdata;
                //printf("%s", address_string);
                return;
            }
            if (tcp_connection->client.count_new)
            {
                /* 表示客户端接收到新的数据 */
                hlf = &tcp_connection->client;
                /* 此时hlf表示的是客户端的TCP连接信息 */
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                //sprintf(address_string + strlen(address_string), ":%i", ip_and_port.source);
                strcat(address_string, " <--- ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                //sprintf(address_string + strlen(address_string), ":%i", ip_and_port.dest);
                strcat(address_string, "\n");
                //printf("--------------------------------\n");
                //printf("%s", address_string);
                memcpy(content, hlf->data, hlf->count_new);
                content[hlf->count_new] = '\0';
                //printf("客户端接收数据\n");
                //for (i = 0; i < hlf->count_new; i++) {
                //    printf("%s", char_to_ascii(content[i]));
                //    // 输出客户端接收的新的数据，以可打印字符进行显示
                //}
                //printf("\n");
            }
            else
            {
                /* 表示服务器端接收到新的数据 */
                hlf = &tcp_connection->server;
                /* 此时hlf表示服务器端的TCP连接信息 */
                strcpy(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.saddr))));
                //sprintf(address_string + strlen(address_string), ":%i", ip_and_port.source);
                strcat(address_string, " ---> ");
                strcat(address_string, inet_ntoa(*((struct in_addr *)&(ip_and_port.daddr))));
                //sprintf(address_string + strlen(address_string), ":%i", ip_and_port.dest);
                strcat(address_string, "\n");
                //printf("--------------------------------\n");
                //printf("%s", address_string);
                memcpy(content, hlf->data, hlf->count_new);
                content[hlf->count_new] = '\0';
                //printf("服务器端接收数据:\n");
                //for (i = 0; i < hlf->count_new; i++) {
                //    printf("%s", char_to_ascii(content[i]));
                // 输出服务器接收到的新的数据
                //}
                //printf("\n");
            }
        }
    default:
        break;
    }
    return;
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

int main(int argc, char *argv[], char **env)
{
    struct nids_chksum_ctl temp;
    temp.netaddr = 0;
    temp.mask = 0;
    temp.action = 1;

    nids_register_chksum_ctl(&temp, 1);
    /*这段是相关与计算校验和的，比较新的网卡驱动会自动计算校验和，我们要做的就是把它关掉*/
    nids_params.filename = "/dev/shm/huawei_tcp.pcap";
    // nids_params.device = "all";
    if (!nids_init()) /* Libnids初始化 */
    {
        printf("Error：%s\n", nids_errbuf);
        exit(1);
    }
    nids_register_tcp((void *)tcp_protocol_callback);
    nids_run_worker();
    return 0;
}
