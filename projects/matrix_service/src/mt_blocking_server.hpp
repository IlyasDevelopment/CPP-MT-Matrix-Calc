#pragma once

#include "server.hpp"
#include "my_socket.hpp"

#include <thread>

namespace matrix_service {

    class MtBlockingServer : public Server
    {
    public:
        explicit MtBlockingServer(Config conf);
        MtBlockingServer(const MtBlockingServer&) = delete;
        MtBlockingServer& operator=(const MtBlockingServer&) = delete;

        MtBlockingServer(MtBlockingServer&& another) noexcept;
        MtBlockingServer& operator=(MtBlockingServer&& another) noexcept;

        ~MtBlockingServer() override;

        void Run() override;

    private:
        void on_stop();

        void swap_servers(MtBlockingServer& another);

        void accept_loop();

        void handle_client(MySocket client_socket);

        template<typename IOFunc>
        bool try_io_enough(MySocket& sock, std::size_t required_size, char* buff, IOFunc io_func);

    private:
        MySocket server_socket_;

        std::atomic<bool> stop_flag_{false};

        std::mutex server_mutex_;
    };

} // namespace matrix_service
