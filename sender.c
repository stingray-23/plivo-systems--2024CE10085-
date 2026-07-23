#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sock_in, sock_out;
    struct sockaddr_in addr_in, addr_out;

    sock_in = socket(AF_INET, SOCK_DGRAM, 0);
    sock_out = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(47010);
    addr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(sock_in, (struct sockaddr*)&addr_in, sizeof(addr_in));

    memset(&addr_out, 0, sizeof(addr_out));
    addr_out.sin_family = AF_INET;
    addr_out.sin_port = htons(47001);
    addr_out.sin_addr.s_addr = inet_addr("127.0.0.1");

    uint32_t seq;
    uint8_t payload[160];
    uint8_t prev_payload[160];
    int has_prev = 0;
    uint8_t buf[164];

    while (1) {
        int len = recv(sock_in, buf, sizeof(buf), 0);
        if (len == 164) {
            seq = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
            memcpy(payload, buf + 4, 160);

            // Send the standard frame immediately
            sendto(sock_out, buf, 164, 0, (struct sockaddr*)&addr_out, sizeof(addr_out));

            // Every 2 frames, send a 3rd recovery packet (XOR of current and previous)
            if (seq % 2 == 1 && has_prev) {
                uint8_t fec_buf[164];
                uint32_t block_seq = (seq / 2) | 0x80000000;
                fec_buf[0] = (block_seq >> 24) & 0xFF;
                fec_buf[1] = (block_seq >> 16) & 0xFF;
                fec_buf[2] = (block_seq >> 8) & 0xFF;
                fec_buf[3] = block_seq & 0xFF;
                
                for (int i = 0; i < 160; i++) {
                    fec_buf[4 + i] = payload[i] ^ prev_payload[i];
                }
                sendto(sock_out, fec_buf, 164, 0, (struct sockaddr*)&addr_out, sizeof(addr_out));
            }
            
            memcpy(prev_payload, payload, 160);
            has_prev = 1;
        }
    }
    return 0;
}