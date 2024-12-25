#pragma once

#include <sys/socket.h> // SHUT_RDWR

namespace matrix_service
{

    class MySocket
    {
    public:
        MySocket();
        explicit MySocket(int fd);
        ~MySocket();

        MySocket(const MySocket&) = delete;
        MySocket& operator=(const MySocket&) = delete;

        MySocket(MySocket&& other) noexcept;
        MySocket& operator=(MySocket&& other) noexcept;

        explicit operator bool() const;

        int get() const;

        // Закрыть текущий сокет (если есть) и принять управление новым fd
        void reset(int new_fd = -1);

        // Отвязать сокет от RAII (возвращает fd, а this сбрасывает в -1)
        int release();

    private:
        // Закрыть, если валидный (shutdown + close)
        void close_if_valid();

    private:
        int fd_;
    };

} // namespace matrix_service
