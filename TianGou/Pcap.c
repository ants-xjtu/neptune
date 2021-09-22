#include "TianGou.h"

int pcap_setfilter(pcap_t *p, struct bpf_program *fp)
{
    MESSAGE();
    return 0;
}

int pcap_get_selectable_fd(pcap_t *p)
{
    MESSAGE("return -1");
    return -1;
}

void pcap_close(pcap_t *p)
{
    MESSAGE();
}

char *pcap_geterr(pcap_t *p)
{
    MESSAGE("return \"you are in a MOON\"");
    return "you are in a MOON";
}

pcap_t *pcap_open_offline(const char *fname, char *errbuf)
{
    MESSAGE("fname = %s", fname);
    return (pcap_t *)0x42;
}

int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    MESSAGE("callback = %p", callback);
    return (interface.pcapLoop)(p, cnt, callback, user);
}

const u_char *pcap_next(pcap_t *p, struct pcap_pkthdr *h)
{
    // this will be called a lot, so no MESSAGE for it
    return (interface.pcapNext)(p, h);
}

int pcap_compile(pcap_t *p, struct bpf_program *fp,
                 const char *str, int optimize, bpf_u_int32 netmask)
{
    MESSAGE("return 0");
    return 0;
}

char *default_dev = "eno1"; // modify this if things went wrong

char *pcap_lookupdev(char *errbuf)
{
    MESSAGE("lookupdev return default device");
    // strncpy(errbuf, "not supported", PCAP_ERRBUF_SIZE);
    return default_dev;
}

pcap_t *pcap_open_live(const char *device, int snaplen,
                       int promisc, int to_ms, char *errbuf)
{
    MESSAGE("return 0x42");
    // strncpy(errbuf, "not supported", PCAP_ERRBUF_SIZE);
    return (pcap_t *)0x42;
}

int pcap_datalink(pcap_t *p)
{
    // MESSAGE("return DLT_EN10MB");
    return DLT_EN10MB;
}

int pcap_dispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    return (interface.pcapDispatch)(p, cnt, callback, user);
}

int pcap_stats(pcap_t *p, struct pcap_stat *pt)
{
    MESSAGE("return 1");
    return 1;
}

pcap_t *pcap_create(const char *source, char *errbuf)
{
    MESSAGE("source = %s", source);
    return (pcap_t *)0x42;
}

int pcap_activate(pcap_t *p)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_set_buffer_size(pcap_t *p, int buffer_size)
{
    MESSAGE("buffer_size = %d", buffer_size);
    return 0;
}

int pcap_inject(pcap_t *p, const void *buf, size_t size)
{
    // MESSAGE("not implemented");
    // abort();
    return (int )size;
}

int pcap_set_snaplen(pcap_t *p, int snaplen)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_set_promisc(pcap_t *p, int promisc)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_set_timeout(pcap_t *p, int to_ms)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_set_tstamp_precision(pcap_t *p, int tstamp_precision)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_set_immediate_mode(pcap_t *p, int immediate_mode)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_setnonblock(pcap_t *p, int nonblock, char *errbuf)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_fileno(pcap_t *p)
{
    MESSAGE("return 42");
    return 42;
}

int pcap_get_tstamp_precision(pcap_t *p)
{
    MESSAGE("return PCAP_TSTAMP_PRECISION_MICRO");
    return PCAP_TSTAMP_PRECISION_MICRO;
}

int pcap_setdirection(pcap_t *p, pcap_direction_t d)
{
    MESSAGE("return 0");
    return 0;
}

int pcap_lookupnet(
    const char *device, bpf_u_int32 *netp,
    bpf_u_int32 *maskp, char *errbuf)
{
    MESSAGE("device = %s", device);
    return 0;
}

void
pcap_freecode(struct bpf_program *program)
{
    MESSAGE("return");
    return;
}