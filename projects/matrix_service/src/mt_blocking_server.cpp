#include "mt_blocking_server.hpp"

#include "utility.hpp"            // VALIDATE_LINUX_CALL, RaiseLinuxCallError
#include "executor/executor.hpp"  // ExecuteProcedure

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <iostream>
#include <thread>

namespace matrix_service
{
    MtBlockingServer::MtBlockingServer(Config conf)
        : Server(std::move(conf))
    {
        int raw_fd = -1;
        VALIDATE_LINUX_CALL(raw_fd = ::socket(AF_INET, SOCK_STREAM, 0));
        server_socket_.reset(raw_fd);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        VALIDATE_LINUX_CALL(::inet_pton(AF_INET, Cfg().listening_address.c_str(), &addr.sin_addr));
        addr.sin_port = htons(Cfg().port);

        VALIDATE_LINUX_CALL(::bind(server_socket_.get(),
                                   reinterpret_cast<struct sockaddr*>(&addr),
                                   sizeof(addr)));

        constexpr int queue_size = 5;
        ::listen(server_socket_.get(), queue_size);
    }

    MtBlockingServer::MtBlockingServer(MtBlockingServer&& another) noexcept
        : Server(std::move(another))
    {
        swap_servers(another);
    }

    MtBlockingServer& MtBlockingServer::operator=(MtBlockingServer&& another) noexcept
    {
        static_cast<Server&>(*this) = std::move(another);
        swap_servers(another);
        return *this;
    }

    MtBlockingServer::~MtBlockingServer()
    {
        // RAII-сокет автоматически закрывает дескриптор при уничтожении
    }

    void MtBlockingServer::on_stop()
    {
        stop_flag_.store(true);
        std::lock_guard<std::mutex> lock(server_mutex_);
        if (server_socket_) {
            ::shutdown(server_socket_.get(), SHUT_RDWR);
        }
    }

    void MtBlockingServer::Run()
    {
        accept_loop();
    }

    void MtBlockingServer::accept_loop()
    {
        while (!stop_flag_) {
            int client_fd = ::accept(server_socket_.get(), nullptr, nullptr);
            if (client_fd == -1) {
                if (!stop_flag_) {
                    RaiseLinuxCallError(__LINE__, __FILE__, "accept", "in MtBlockingServer::accept_loop");
                } else {
                    break;
                }
            } else {
                MySocket client_socket(client_fd);
                std::thread t(&MtBlockingServer::handle_client, this, std::move(client_socket));
                t.detach();
            }
        }
    }

    void MtBlockingServer::handle_client(MySocket client_socket)
    {
        while (!stop_flag_) {
            int content_size = 0;
            if (!try_io_enough(client_socket,
                               sizeof(content_size),
                               reinterpret_cast<char*>(&content_size),
                               &read))
            {
                break;
            }

            if (content_size == 0) {
                continue;
            }

            std::string request(content_size, '\0');
            if (!try_io_enough(client_socket, content_size, &request[0], &read)) {
                break;
            }

            // Обрабатываем
            auto result = ExecuteProcedure(request);

            // Записываем ответ
            int resp_size = static_cast<int>(result.first.size());
            if (!try_io_enough(client_socket,
                               sizeof(resp_size),
                               reinterpret_cast<char*>(&resp_size),
                               &write) ||
                !try_io_enough(client_socket,
                               resp_size,
                               const_cast<char*>(result.first.data()),
                               &write))
            {
                break;
            }

            if (!result.second) {
                // нет keepalive
                break;
            }
        }
        // RAII сокет (client_socket) закроется в деструкторе
    }

    template<typename IOFunc>
    bool MtBlockingServer::try_io_enough(MySocket& sock, std::size_t required_size, char* buff, IOFunc io_func)
    {
        std::size_t processed_cnt = 0;
        while (processed_cnt < required_size) {
            int res = io_func(sock.get(), buff + processed_cnt, required_size - processed_cnt);
            if (res > 0) {
                processed_cnt += res;
            } else if (res == 0) {
                // клиент закрыл соединение
                break;
            } else {
                if (!stop_flag_) {
                    RaiseLinuxCallError(__LINE__, __FILE__, "read/write()", "in MtBlockingServer::try_io_enough");
                } else {
                    // Прервано остановкой
                    assert(errno == EINTR || errno == EINVAL);
                }
                break;
            }
        }

        return (processed_cnt == required_size) && (!stop_flag_);
    }

    void MtBlockingServer::swap_servers(MtBlockingServer& another)
    {
        std::swap(server_socket_, another.server_socket_);
    }
} // namespace matrix_service
