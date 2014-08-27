#include <getopt.h>
#include <unordered_map>
#include <vector>
#include <string>

#include "StringSplit.h"
#include "MQTTWrapper.h"
#include "RF24NetworkWrapper.h"
#include "RF24Node_types.h"
#include "RF24Node.h"

/*
 * Main Program
 */
int main(int argc, char *argv[]) {
    std::string id = "RF24Node";
    std::string host = "localhost";
    int port = 1883;
    rf24_pa_dbm_e palevel = RF24_PA_MAX;
    rf24_datarate_e datarate = RF24_250KBPS;
    uint8_t channel = 0x4c;
    uint16_t node_address = 00;
    char key[16] = { 
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    bool debug = false;

    static struct option long_options[] = {
      {"datarate", required_argument, nullptr, 'd'},
      {"palevel", required_argument, nullptr, 'p'},
      {"channel", required_argument, nullptr, 'c'},
      {"node", required_argument, nullptr, 'n'},
      {"key", required_argument, nullptr, 'k'},
      {"verbose", no_argument, nullptr, 'v'},
      {nullptr, 0, nullptr, 0}
    };

    std::vector<std::string> key_elements;
    int opt = 0, long_index = 0;
    while ((opt = getopt_long(argc, argv,"n:c:d:p:k:v", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'n' : 
                if (debug) printf ("option -n with value '%s'\n", optarg);
                node_address = std::stoul(optarg, nullptr, 0);
                break;
            case 'c' : 
                if (debug) printf ("option -c with value '%s'\n", optarg);
                channel = std::stoul(optarg, nullptr, 0);
                break;
            case 'd':
                if (debug) printf ("option -d with value '%s'\n", optarg);
                datarate = static_cast<rf24_datarate_e>(std::stoi(optarg, nullptr, 0));
                break;
            case 'p':
                if (debug) printf ("option -p with value '%s'\n", optarg);
                palevel = static_cast<rf24_pa_dbm_e>(std::stoi(optarg, nullptr, 0));
                break;
            case 'k':
                if (debug) printf ("option -k with value '%s'\n", optarg);
                key_elements = split(optarg, ' ');

                for (unsigned int i = 0; i < key_elements.size(); i++) {
                    key[i] = std::stoul(key_elements[i].c_str(), nullptr, 0);
                }
                break;
            case 'v':
                if (debug) printf ("option -v\n");
                debug = true;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    MQTTWrapper msg_proto(id.c_str(), host.c_str(), port);
    RF24NetworkWrapper network(channel, node_address, palevel, datarate);
    RF24Node node(network, msg_proto, key);
    node.set_debug(debug);

    node.begin();
    while(true) {
        node.loop();
    }
    node.end();

    return 0;
}
