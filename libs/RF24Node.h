#ifndef RF24Node_H
#define RF24Node_H

#include <functional>
#include <mosquittopp.h>

// Challenge Request Structure
struct C_request_message_t {
    time_t challenge;
};

// Challenge Command Structure
struct C_command_message_t {
    unsigned char hash[8];
    uint8_t command;
};

// Timesync Structure
struct T_message_t {
  time_t timestamp; 
};

// Configure an RF24Node
struct RF24Node_Config {
    std::string id;
    std::string host;
    int port;
    rf24_pa_dbm_e palevel;
    rf24_datarate_e datarate;
    uint8_t channel;
    uint16_t node_address;
    char key[16];
};

typedef std::unordered_map<uint16_t /* node_address */, uint8_t /* command */> command_map;

typedef std::function<void(const struct mosquitto_message *message)> on_message_callback;
class MQTTWrapper: public  mosqpp::mosquittopp {
    public:
        MQTTWrapper(const char *id, const char *host, int port) : mosqpp::mosquittopp(id, false), host(host), port(port) {}

        void begin(on_message_callback cb) {
            mosqpp::lib_init();
            this->cb = cb;
            this->connect(this->host, this->port, 60 /* keepalive */);
            this->subscribe(nullptr, "/ipc/rf24_node");
        }

        void end(void) {
            mosqpp::lib_cleanup();
        }

        void set_callback(on_message_callback cb) {
            this->cb = cb;
        }

    protected:
        const char *host;
        int port;
        on_message_callback cb;
        void on_message(const struct mosquitto_message *message) {
            this->cb(message);
        }
};

class RF24Node {
  protected:
    command_map queued_commands;

    MQTTWrapper mqtt;
    RF24 radio;
    RF24Network network;

    rf24_pa_dbm_e palevel;
    rf24_datarate_e datarate;
    uint8_t channel;
    uint16_t node_address;
    char key[16];

    void handle_challenge(RF24NetworkHeader& header);
    void handle_timesync(RF24NetworkHeader& header);
    void handle_republish(RF24NetworkHeader& header);

    void generateSipHash(time_t challenge, unsigned char (&hash)[8]);

  public:
    RF24Node(RF24Node_Config config);

    void begin(void);
    void end(void);
    void loop(void);

    bool write(RF24NetworkHeader& header,const void* message, size_t len);

    void on_message(const struct mosquitto_message *message);
};

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


#endif
