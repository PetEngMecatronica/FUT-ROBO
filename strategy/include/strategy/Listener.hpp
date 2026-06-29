#pragma once

#include <asio.hpp>

class Listener {
public:
    Listener(std::string_view address, int port);
    size_t receive(uint8_t* buffer, size_t max_size);

private:
    asio::io_context m_io_context;
    asio::ip::udp::socket m_socket;
};
