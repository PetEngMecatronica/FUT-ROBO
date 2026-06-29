#include "strategy/Listener.hpp"

#include <print>

using asio::ip::udp;

Listener::Listener(std::string_view multicast_address, int port)
    : m_io_context(), m_socket(m_io_context)
{
    auto address = asio::ip::make_address(multicast_address);

    // Open the socket matching the protocol version
    m_socket.open(udp::v4());

    // CRITICAL: Allow multiple listeners to bind to this port simultaneously
    m_socket.set_option(udp::socket::reuse_address(true));

    // Bind to the local port (using the "any" address wildcard 0.0.0.0)
    m_socket.bind(udp::endpoint(udp::v4(), port));

    // Join the multicast group
    // (forces the NIC to accept packets sent to this group IP)
    m_socket.set_option(asio::ip::multicast::join_group(address));
}

size_t Listener::receive(uint8_t* out_buffer, size_t max_size) {
    asio::error_code ec;
    udp::endpoint sender_endpoint;

    // Receive data synchronously (blocking call)
    size_t bytes_received = m_socket.receive_from(
        asio::buffer(out_buffer, max_size),
        sender_endpoint,
        0,
        ec
    );

    if (ec) {
        std::println("Error receiving data: {}", ec.message());
        return 0;
    }

    return bytes_received;
}
