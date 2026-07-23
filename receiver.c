#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

struct frame_t {
    int valid;
    uint8_t payload[160];
};

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

int main() {
    int sock_in, sock_out;
    struct sockaddr_in addr_in, addr_out;

    sock_in = socket(AF_INET, SOCK_DGRAM, 0);
    sock_out = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(47002);
    addr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(sock_in, (struct sockaddr*)&addr_in, sizeof(addr_in));

    memset(&addr_out, 0, sizeof(addr_out));
    addr_out.sin_family = AF_INET;
    addr_out.sin_port = htons(47020);
    addr_out.sin_addr.s_addr = inet_addr("127.0.0.1");

    char *t0_env = getenv("T0");
    char *delay_env = getenv("DELAY_MS");
    if (!t0_env || !delay_env) return 1;
    
    double t0 = strtod(t0_env, NULL);
    double delay_ms = strtod(delay_env, NULL);
    
    struct frame_t *frames = calloc(65536, sizeof(struct frame_t));
    struct frame_t *fecs = calloc(65536, sizeof(struct frame_t));
    uint32_t expected_seq = 0;

    while (1) {
        if (!frames[expected_seq].valid) {
            uint32_t block = expected_seq / 2;
            uint32_t other = (expected_seq % 2 == 0) ? (expected_seq + 1) : (expected_seq - 1);
            if (fecs[block].valid && frames[other].valid) {
                for (int i = 0; i < 160; i++) {
                    frames[expected_seq].payload[i] = fecs[block].payload[i] ^ frames[other].payload[i];
                }
                frames[expected_seq].valid = 1;
            }
        }

        if (frames[expected_seq].valid) {
            uint8_t out[164];
            out[0] = (expected_seq >> 24) & 0xFF;
            out[1] = (expected_seq >> 16) & 0xFF;
            out[2] = (expected_seq >> 8) & 0xFF;
            out[3] = expected_seq & 0xFF;
            memcpy(out + 4, frames[expected_seq].payload, 160);
            sendto(sock_out, out, 164, 0, (struct sockaddr*)&addr_out, sizeof(addr_out));
            expected_seq++;
            continue;
        }

        double now = get_time();
        double deadline = t0 + (delay_ms / 1000.0) + (expected_seq * 0.020) - 0.001; 

        if (now >= deadline) {
            expected_seq++; 
            continue;
        }

        double wait = deadline - now;
        struct timeval tv;
        tv.tv_sec = (int)wait;
        tv.tv_usec = (int)((wait - tv.tv_sec) * 1000000.0);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_in, &readfds);
        
        int ret = select(sock_in + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sock_in, &readfds)) {
            uint8_t buf[164];
            int len = recv(sock_in, buf, sizeof(buf), 0);
            if (len == 164) {
                uint32_t header = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
                int is_fec = (header & 0x80000000) != 0;
                uint32_t seq = header & 0x7FFFFFFF;
                
                if (seq < 65536) {
                    if (is_fec) {
                        fecs[seq].valid = 1;
                        memcpy(fecs[seq].payload, buf + 4, 160);
                    } else {
                        frames[seq].valid = 1;
                        memcpy(frames[seq].payload, buf + 4, 160);
                    }
                }
            }
        }
    }
    return 0;
}