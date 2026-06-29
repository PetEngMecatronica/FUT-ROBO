#pragma once

#include <asio.hpp>

class Publisher {
public:
    Publisher(std::string_view multicast_address, int port);
    void send(const uint8_t* data, size_t size);

private:
    asio::io_context m_io_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_endpoint;
};
