#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include <mqtt/async_client.h>

class Publisher {
public:
    // const std::string SERVER_ADDRESS = "tcp://localhost:1883";
    // const std::string CLIENT_ID = "MANAGER";
    // const std::string TOPIC("test/topic");
    const int QOS = 1;
    // const int TIMEOUT = 10000;

    Publisher(const std::string& client_id, const std::string& server_addr, int qos = 1);
    void publish(const std::string& msg, const std::string& topic);

    void qos(int new_value);
    int qos() const;

private:
    class Callback : public virtual mqtt::callback {
    public:
        void connection_lost(const std::string& cause) override;
        // void delivery_complete(mqtt::delivery_token_ptr token) override;
    };

    mqtt::async_client client_;
    Callback callback_;
    int qos_;
};

#endif // PUBLISHER_HPP
