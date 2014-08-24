#include <getopt.h>
#include "StringSplit.h"
#include "RF24Node_types.h"
#include "RF24Node.h"

/*
 * Main Program
 */
int main(int argc, char *argv[]) {
    RF24Node_Config config = {
        "RF24Node",
        "localhost",
        1883,
        RF24_PA_MAX,
        RF24_250KBPS,
        0x4c,
        00,
        { 
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
        },
        false
    };

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
                if (config.debug) printf ("option -n with value '%s'\n", optarg);
                config.node_address = std::stoul(optarg, nullptr, 0);
                break;
            case 'c' : 
                if (config.debug) printf ("option -c with value '%s'\n", optarg);
                config.channel = std::stoul(optarg, nullptr, 0);
                break;
            case 'd':
                if (config.debug) printf ("option -d with value '%s'\n", optarg);
                config.datarate = static_cast<rf24_datarate_e>(std::stoi(optarg, nullptr, 0));
                break;
            case 'p':
                if (config.debug) printf ("option -p with value '%s'\n", optarg);
                config.palevel = static_cast<rf24_pa_dbm_e>(std::stoi(optarg, nullptr, 0));
                break;
            case 'k':
                if (config.debug) printf ("option -k with value '%s'\n", optarg);
                key_elements = split(optarg, ' ');

                for (unsigned int i = 0; i < key_elements.size(); i++) {
                    config.key[i] = std::stoul(key_elements[i].c_str(), nullptr, 0);
                }
                break;
            case 'v':
                if (config.debug) printf ("option -v\n");
                config.debug = true;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    RF24Node node(config);

    node.begin();
    while(true) {
        node.loop();
    }
    node.end();

    return 0;
}
