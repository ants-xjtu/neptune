#include <cstdio>
#include <vector>

#include <pcap/pcap.h>

using namespace std;

void onPacket(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
    //
}

int main(int argc, char *argv[]) {
    printf("[test++] start\n");
    unsigned char *buffer = new unsigned char[128];
    printf("[test++] `new` return: %p\n", buffer);
    pcap_loop(nullptr, -1, onPacket, nullptr);
    return 0;
}
