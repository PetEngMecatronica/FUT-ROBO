#include "futbot/Publisher.hpp"

#include <print>

using asio::ip::udp;

Publisher::Publisher(std::string_view multicast_address, int port)
    : m_io_context(), m_socket(m_io_context, udp::v4())
{
    // Converts the IP address string to an ASIO address object
    auto address = asio::ip::make_address(multicast_address);

    // Define the destination endpoint (Group Address + Port)
    m_endpoint = udp::endpoint(address, port);

    // Set the TTL (Time-To-Live) to 1 (restricted to the local network)
    asio::ip::multicast::hops ttl(1);
    m_socket.set_option(ttl);
}

void Publisher::send(const uint8_t* data, size_t size) {
    asio::error_code ec;

    m_socket.send_to(asio::buffer(data, size), m_endpoint, 0, ec);

    if (ec) {
        std::println("Couldn't send data: {}", ec.message());
    }
}
