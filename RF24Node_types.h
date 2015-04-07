#pragma once

#include <string>
#include <stdint.h>
#include <ctime>
#include <unordered_map>

typedef std::unordered_map<uint8_t /* message_type */, std::string /* payload */> typepayload_map;
typedef std::unordered_map<uint16_t /* node_address */, typepayload_map> payload_map;


/* 
 * Below is compat w/RF24SensorNet 
 */

/* Defined packet types */
enum pkt_type {
  PKT_INFO   = 0, /* General info for this node */
  PKT_POWER  = 1, /* Power status for this node */
  PKT_SWITCH = 2, /* Digital (on/off) switches */
  PKT_RGB    = 3, /* RGB (dimmer) switches */
  PKT_TEMP   = 4, /* Temperature sensors */
  PKT_HUMID  = 5,  /* Humidity sensors */
  PKT_MOISTURE = 6, /* Soil moisture sensors */
  PKT_ENERGY = 7, /* Energy readings sensors */
  PKT_TIME = 8,  /* Time */
  PKT_CHALLENGE = 9,  /* Challenge */
};

/* Packet containing info about this node's power supply. */
struct pkt_power_t {
  bool battery; /* Is this node battery powered? */
  bool solar; /* Does this node have a solar panel? */
  uint16_t vcc; /* Supply voltage */
  uint16_t vs; /* Voltage supplied by solar panel. 0 if not panel. */
  uint16_t id; /* ID for the battery, unique to this node */
};

/* Packet for an on-off switch attached to this node. */
struct pkt_switch_t {
  unsigned char hash[8]; /* Siphash */
  uint16_t id; /* ID for the switch, unique to this node. */
  bool state; /* Is the switch on? */
  uint32_t timer; /* Seconds remaining until timer deactivates switch. 0 for no timer. */
};

/* Packet for a dimmer or RGB light attached to this node. */
struct pkt_rgb_t {
  unsigned char hash[8]; /* Siphash */
  uint16_t id; /* ID for the switch, unique to this node. */
  char rgb[3]; /* 0-255 for RGB. Off is [0, 0, 0] */
  uint32_t timer; /* Seconds remaining until timer deactivates switch. 0 for no timer. */
};

/* Packet for readings from a temperature sensor. */
struct pkt_temp_t {
  uint16_t id; /* ID for the sensor, unique to this node. */
  int16_t temp; /* Temperature reading from the sensor, in 0.1 degree C. */
};

/* Packet for readings from a humidity sensor. */
struct pkt_humid_t {
  uint16_t id; /* ID for the sensor, unique to this node. */
  uint16_t humidity; /* Relative humidity from the sensor, in 0.1%. */
};

/* Packet for readings from a soil moisture sensor. */
struct pkt_moisture_t {
  uint16_t id; /* ID for the sensor, unique to this node. */
  uint16_t moisture; /* Soil moisture from the sensor. */
};

/* Packet for readings from an energy sensor. */
struct pkt_energy_t {
  uint16_t id; /* ID for the sensor, unique to this node. */
  uint16_t energy; /* Energy from the sensor, in 0.1%. */
};

/* Packet for reading/receiving challenges. */
struct pkt_challenge_t {
    time_t challenge;
    uint8_t type;
};

/* Packet for time sync. */
struct pkt_time_t {
    time_t timestamp;
};
