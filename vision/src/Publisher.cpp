#include "futbot/Publisher.hpp"

#include <print>

Publisher::Publisher(const std::string& client_id, const std::string& server_addr, int qos)
: client_{server_addr, client_id}, qos_{qos}
{
    mqtt::connect_options conn_opts;

    conn_opts.set_keep_alive_interval(20);
    conn_opts.set_clean_session(true);

    try {
        client_.set_callback(callback_);

        mqtt::token_ptr connection_token = client_.connect(conn_opts);
        connection_token->wait();
    } catch (const mqtt::exception& e) {
        std::println(std::cerr, "MQTT Exception: {}", e.what());
        exit(EXIT_FAILURE);
    }
}

void Publisher::publish(const std::string& msg, const std::string& topic)
{
    try {
        mqtt::message_ptr pub_msg = mqtt::make_message(topic, msg, qos_, false);
        client_.publish(pub_msg)->wait();
    } catch (const mqtt::exception& e) {
        std::println(std::cerr, "MQTT Exception: {}", e.what());
        exit(EXIT_FAILURE);
    }
}

void Publisher::qos(int new_value)
{
    qos_ = new_value;
}

int Publisher::qos() const
{
    return qos_;
}

void Publisher::Callback::connection_lost(const std::string& cause)
{
    std::println("Connection lost: {}", cause);
}

// void Publisher::Callback::delivery_complete(mqtt::delivery_token_ptr token)
// {
//     (void)token; // Prevent unused variable warning
//     std::println("Message delivered");
// }
