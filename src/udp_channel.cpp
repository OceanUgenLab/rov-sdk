// ou_sdk UDP 字节通道实现
#include "ou/udp_channel.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ou {

struct UdpChannel::Impl {
#ifdef _WIN32
    SOCKET sock = INVALID_SOCKET;
#else
    int sock = -1;
#endif
    bool open = false;
    uint16_t local_port = 0;
    bool peer_known = false;
    std::string peer_ip;
    uint16_t peer_port = 0;
    sockaddr_in peer_addr{};
};

namespace {

#ifdef _WIN32
bool sock_valid(SOCKET s) { return s != INVALID_SOCKET; }
void sock_close(SOCKET &s) {
    if (s != INVALID_SOCKET) {
        ::closesocket(s);
        s = INVALID_SOCKET;
    }
}
#else
bool sock_valid(int s) { return s >= 0; }
void sock_close(int &s) {
    if (s >= 0) {
        ::close(s);
        s = -1;
    }
}
#endif

int ms_to_poll(std::chrono::milliseconds timeout) {
    const auto ms = timeout.count();
    if (ms <= 0) return 0;
    if (ms > static_cast<std::chrono::milliseconds::rep>(INT_MAX)) return INT_MAX;
    return static_cast<int>(ms);
}

}  // namespace

UdpChannel::UdpChannel() : impl_(new Impl{}) {}

UdpChannel::~UdpChannel() {
    if (impl_) {
        sock_close(impl_->sock);
        delete impl_;
        impl_ = nullptr;
    }
}

bool UdpChannel::bind(uint16_t local_port) {
    if (!impl_) return false;
    if (impl_->open) return true;

#ifdef _WIN32
    static const bool wsa_ok = [] {
        WSADATA data{};
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    if (!wsa_ok) return false;
    impl_->sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    impl_->sock = ::socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (!sock_valid(impl_->sock)) return false;

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(local_port);

    if (::bind(impl_->sock, reinterpret_cast<const sockaddr *>(&local_addr),
               sizeof(local_addr)) != 0) {
        std::fprintf(stderr, "[UdpChannel] bind port %u failed: %d\n", local_port,
#ifdef _WIN32
                     WSAGetLastError()
#else
                     errno
#endif
        );
        sock_close(impl_->sock);
        return false;
    }

    impl_->open = true;
    impl_->local_port = local_port;
    return true;
}

bool UdpChannel::set_peer(std::string ip, uint16_t port) {
    if (!impl_ || !impl_->open) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) return false;

    impl_->peer_addr = addr;
    impl_->peer_ip = std::move(ip);
    impl_->peer_port = port;
    impl_->peer_known = true;
    return true;
}

bool UdpChannel::is_open() const {
    return impl_ && impl_->open;
}

bool UdpChannel::send(std::span<const uint8_t> data) {
    if (!impl_ || !impl_->open || data.empty()) return false;
    if (!impl_->peer_known) return false;

    const auto *addr = reinterpret_cast<const sockaddr *>(&impl_->peer_addr);
    const auto addrlen = static_cast<socklen_t>(sizeof(impl_->peer_addr));

#ifdef _WIN32
    const int n = ::sendto(impl_->sock,
                           reinterpret_cast<const char *>(data.data()),
                           static_cast<int>(data.size()), 0, addr, addrlen);
#else
    const ssize_t n = ::sendto(impl_->sock, data.data(), data.size(), 0,
                               addr, addrlen);
#endif
    return n == static_cast<std::make_signed_t<size_t>>(data.size());
}

std::optional<std::vector<uint8_t>> UdpChannel::recv_bytes(
    std::chrono::milliseconds timeout) {
    if (!impl_ || !impl_->open) return std::nullopt;

    const auto to_ms = ms_to_poll(timeout);

#ifdef _WIN32
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(impl_->sock, &readfds);
    timeval tv{};
    tv.tv_sec = to_ms / 1000;
    tv.tv_usec = (to_ms % 1000) * 1000;
    const int rc = ::select(0, &readfds, nullptr, nullptr, &tv);
    if (rc <= 0) return std::nullopt;
    if (!FD_ISSET(impl_->sock, &readfds)) return std::nullopt;
#else
    pollfd pfd{};
    pfd.fd = impl_->sock;
    pfd.events = POLLIN;
    const int rc = ::poll(&pfd, 1, to_ms);
    if (rc <= 0) return std::nullopt;
    if (!(pfd.revents & POLLIN)) return std::nullopt;
#endif

    std::vector<uint8_t> buf(65535);
    sockaddr_in from_addr{};
    socklen_t from_len = sizeof(from_addr);

#ifdef _WIN32
    const int n = ::recvfrom(impl_->sock,
                             reinterpret_cast<char *>(buf.data()),
                             static_cast<int>(buf.size()), 0,
                             reinterpret_cast<sockaddr *>(&from_addr),
                             &from_len);
#else
    const ssize_t n = ::recvfrom(impl_->sock, buf.data(), buf.size(), 0,
                                 reinterpret_cast<sockaddr *>(&from_addr),
                                 &from_len);
#endif
    if (n < 0) return std::nullopt;
    buf.resize(static_cast<size_t>(n));

    // 学习对端地址（首个数据报的源地址）
    char ip_str[INET_ADDRSTRLEN] = {};
    if (inet_ntop(AF_INET, &from_addr.sin_addr, ip_str, sizeof(ip_str))) {
        impl_->peer_addr = from_addr;
        impl_->peer_ip = ip_str;
        impl_->peer_port = ntohs(from_addr.sin_port);
        impl_->peer_known = true;
    }
    return buf;
}

std::optional<std::vector<uint8_t>> UdpChannel::try_recv_bytes() {
    return recv_bytes(std::chrono::milliseconds(0));
}

int UdpChannel::native_handle() const {
    if (!impl_) return -1;
#ifdef _WIN32
    return impl_->open ? static_cast<int>(impl_->sock) : -1;
#else
    return impl_->sock;
#endif
}

}  // namespace ou
