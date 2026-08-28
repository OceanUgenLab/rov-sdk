// ou_sdk 串口字节通道实现
#include "ou/serial_channel.hpp"

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace ou {

struct SerialChannel::Impl {
#ifdef _WIN32
    HANDLE h = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif
    bool open = false;
};

namespace {

#ifndef _WIN32

int baud_to_constant(int baud) {
    switch (baud) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
        default: return -1;
    }
}

int ms_to_poll(std::chrono::milliseconds timeout) {
    const auto ms = timeout.count();
    if (ms <= 0) return 0;
    if (ms > static_cast<std::chrono::milliseconds::rep>(INT_MAX)) return INT_MAX;
    return static_cast<int>(ms);
}

#endif

}  // namespace

SerialChannel::SerialChannel() : impl_(new Impl{}) {}

SerialChannel::~SerialChannel() {
    if (impl_) {
#ifdef _WIN32
        if (impl_->h != INVALID_HANDLE_VALUE) {
            CloseHandle(impl_->h);
            impl_->h = INVALID_HANDLE_VALUE;
        }
#else
        if (impl_->fd >= 0) {
            ::close(impl_->fd);
            impl_->fd = -1;
        }
#endif
        delete impl_;
        impl_ = nullptr;
    }
}

bool SerialChannel::open(std::string device, int baud) {
    if (!impl_) return false;
    if (impl_->open) return true;

#ifdef _WIN32
    // Windows 串口暂为 stub：可编译但返回失败
    (void)device;
    (void)baud;
    return false;
#else
    const int bconst = baud_to_constant(baud);
    if (bconst < 0) return false;

    impl_->fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (impl_->fd < 0) {
        std::fprintf(stderr, "[SerialChannel] open %s failed: %d\n", device.c_str(), errno);
        return false;
    }

    // 清除 O_NONBLOCK，后续超时由 poll 控制
    int flags = ::fcntl(impl_->fd, F_GETFL, 0);
    if (flags >= 0) {
        ::fcntl(impl_->fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    termios tty{};
    if (::tcgetattr(impl_->fd, &tty) != 0) {
        std::fprintf(stderr, "[SerialChannel] tcgetattr failed: %d\n", errno);
        ::close(impl_->fd);
        impl_->fd = -1;
        return false;
    }

    // raw 模式：8N1，禁用流控、echo、信号
    ::cfsetospeed(&tty, static_cast<speed_t>(bconst));
    ::cfsetispeed(&tty, static_cast<speed_t>(bconst));

    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
    tty.c_cflag |= CS8 | CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (::tcsetattr(impl_->fd, TCSANOW, &tty) != 0) {
        std::fprintf(stderr, "[SerialChannel] tcsetattr failed: %d\n", errno);
        ::close(impl_->fd);
        impl_->fd = -1;
        return false;
    }

    ::tcflush(impl_->fd, TCIOFLUSH);
    impl_->open = true;
    return true;
#endif
}

bool SerialChannel::is_open() const {
    return impl_ && impl_->open;
}

bool SerialChannel::send(std::span<const uint8_t> data) {
    if (!impl_ || !impl_->open || data.empty()) return false;

#ifdef _WIN32
    return false;
#else
    const ssize_t n = ::write(impl_->fd, data.data(), data.size());
    return n == static_cast<ssize_t>(data.size());
#endif
}

std::optional<std::vector<uint8_t>> SerialChannel::recv_bytes(
    std::chrono::milliseconds timeout) {
    if (!impl_ || !impl_->open) return std::nullopt;

#ifdef _WIN32
    return std::nullopt;
#else
    pollfd pfd{};
    pfd.fd = impl_->fd;
    pfd.events = POLLIN;

    const int rc = ::poll(&pfd, 1, ms_to_poll(timeout));
    if (rc <= 0) return std::nullopt;
    if (!(pfd.revents & POLLIN)) return std::nullopt;

    std::vector<uint8_t> buf(4096);
    const ssize_t n = ::read(impl_->fd, buf.data(), buf.size());
    if (n <= 0) return std::nullopt;
    buf.resize(static_cast<size_t>(n));
    return buf;
#endif
}

std::optional<std::vector<uint8_t>> SerialChannel::try_recv_bytes() {
    return recv_bytes(std::chrono::milliseconds(0));
}

int SerialChannel::native_handle() const {
    if (!impl_) return -1;
#ifdef _WIN32
    return (impl_->h != INVALID_HANDLE_VALUE) ? static_cast<int>(reinterpret_cast<uintptr_t>(impl_->h)) : -1;
#else
    return impl_->fd;
#endif
}

}  // namespace ou
