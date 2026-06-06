#include "ring_logger.h"
#include <thread>
#include <cstring>
#include <chrono>
#include <string>
#include <cstdio>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static std::atomic<bool> g_running{true};

static void health_server(uint16_t port) {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) return;
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(srv);
        return;
    }
    if (listen(srv, 4) < 0) {
        close(srv);
        return;
    }

    while (g_running.load(std::memory_order_acquire)) {
        int c = accept(srv, nullptr, nullptr);
        if (c < 0) break;
        char buf[1024];
        ssize_t n = recv(c, buf, sizeof(buf)-1, 0);
        if (n > 0) {
            buf[n] = '\0';
            if (std::strncmp(buf, "GET /health", 11) == 0) {
                const char* resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
                send(c, resp, strlen(resp), 0);
            } else {
                const char* resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
                send(c, resp, strlen(resp), 0);
            }
        }
        close(c);
    }
    close(srv);
}

int main() {
    using namespace trading::log;
    RingLogger logger("/tmp/fairtrade.binlog");
    if (!logger.initialize()) {
        std::fprintf(stderr, "Failed to initialize RingLogger\n");
        return 1;
    }

    // start health server on port 8080
    std::thread hs(health_server, static_cast<uint16_t>(8080));
    hs.detach();

    const char* msg = "engine-demo: heartbeat";
    while (g_running.load(std::memory_order_acquire)) {
        logger.log(LogLevel::INFO, reinterpret_cast<const uint8_t*>(msg), static_cast<uint32_t>(std::strlen(msg)));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
