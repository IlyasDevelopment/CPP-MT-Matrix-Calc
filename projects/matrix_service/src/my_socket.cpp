#include "my_socket.hpp"

#include <unistd.h>     // close(), shutdown()
#include <sys/socket.h> // SHUT_RDWR

namespace matrix_service
{

    MySocket::MySocket()
        : fd_(-1)
    {
    }

    MySocket::MySocket(int fd)
        : fd_(fd)
    {
    }

    MySocket::~MySocket()
    {
        close_if_valid();
    }

    MySocket::MySocket(MySocket&& other) noexcept
        : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    MySocket& MySocket::operator=(MySocket&& other) noexcept
    {
        if (this != &other)
        {
            close_if_valid();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    MySocket::operator bool() const
    {
        return fd_ >= 0;
    }

    int MySocket::get() const
    {
        return fd_;
    }

    void MySocket::reset(int new_fd)
    {
        if (fd_ != new_fd)
        {
            close_if_valid();
            fd_ = new_fd;
        }
    }

    int MySocket::release()
    {
        int temp = fd_;
        fd_ = -1;
        return temp;
    }

    void MySocket::close_if_valid()
    {
        if (fd_ >= 0)
        {
            ::shutdown(fd_, SHUT_RDWR);
            ::close(fd_);
            fd_ = -1;
        }
    }

} // namespace matrix_service
