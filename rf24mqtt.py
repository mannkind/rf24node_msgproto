#! .venv/bin/python
#
#   NRF24L01+ to MQTT gateway
#   Copyright (C) 2013 Dustin Brewer
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
The NRF24L01+ to MQTT gateway
"""

__app__ = "NRF24L01+ to MQTT gateway"
__version__ = "0.3.20140331"
__author__ = "Dustin Brewer"
__contact__ = "mannkind@thenullpointer.net"
__copyright__ = "Copyright (C) 2013 Dustin Brewer"
__license__ = 'GPL v3'

import os
import sys
import time
import logging

from libs.daemon import Daemon
from libs.processor import Processor
from libs.config import Config
from libs.mosquitto_wrapper import MosquittoWrapper
from libs.rf24_wrapper import RF24Wrapper

class RF24MQTT(Daemon):
    """
    RF24MQTT daemon.
    Glues the different components together
    """

    def __init__(self, pid_file):
        Daemon.__init__(self, pid_file)

        self.duplicate_check_window = 1
        self.default_topic_pattern = None
        self.publish_undefined_topics = False
        self.logger = None
        self.rf24 = None
        self.mqtt = None
        self.processor = None

        self._routes = {}
        self._actions = {}
        self._topics = {}

    def load(self, devices):
        """
        Read configuration and store bidirectional dicts
        """
        self._routes = {}
        self._actions = {}

        devices = [device for device in devices if 'rf24mqtt' in device]
        for device in devices:
            self._routes[device['id']] = device['topic']

            if type(device['rf24mqtt']) is dict and \
                device['rf24mqtt'].get('controllable', False):

                set_topic = '%s/set' % device['topic']
                self._actions[set_topic] = device['rf24mqtt']['control_values']

    def log(self, level, message):
        """
        If a logger is available, log the message with the given level
        """
        if self.logger:
            self.logger.log(level, message)

    def cleanup(self):
        """
        Clean up connections and unbind ports
        """
        self.log(logging.INFO, "Exiting")
        self.rf24.disconnect()
        self.mqtt.disconnect()
        sys.exit()

    def mqtt_on_message(self, topic, message):
        """
        Message received from a subscribed topic
        """
        self.log(logging.DEBUG, 
            "Message received from MQTT broker: %s %s" % (topic, message))

        if topic == '/ipc/rf24mqtt':
            self.log(logging.DEBUG, 
                "Message received from radio process via MQTT: %s" % message)

            self.rf24_on_message(message)
            return

        action = self._actions.get(topic, {}).get(message, False)
        if action:
            self.log(logging.DEBUG, 
                "Message sent to radio process via MQTT: %s" % action)

            self.mqtt.publish('/ipc/rf24_node', action, False)
            return

    def rf24_on_message(self, received):
        """
        Message from the radio coordinator
        """
        if received is None:
            return 

        self.log(logging.DEBUG, 
            "Received %s (length %s)" % (received, len(received)))

        received = received.strip().rstrip('{')
        if received.count(':') != 1:
            self.log(logging.DEBUG, 'Received data not in the format expected.')
            return 

        (device_id, value) = received.split(':')

        self.log(logging.DEBUG, 
            "Message received from radio: %s %s" % (device_id, value))

        def_top_pat = False
        if self.publish_undefined_topics:
            def_top_pat = self.default_topic_pattern.format(id=device_id)

        topic = self._routes.get((device_id), def_top_pat)
        if topic:
            now = time.time()

            if topic in self._topics.keys():
                dup_t = self._topics[topic]['time'] + self.duplicate_check_window
                match = self._topics[topic]['value'] == value

                if dup_t > now and match:
                    self.log(logging.DEBUG, "Duplicate removed")
                    return

            self._topics[topic] = {'time': now, 'value': value}

            value = self.processor.process(topic, value)
            self.log(logging.INFO, 
                "Sending message to MQTT broker: %s %s" % (topic, value))

            self.mqtt.publish(topic, value, True)

    def run(self):
        """
        Entry point, initiates components and loops forever...
        """
        self.log(logging.INFO, "Starting " + __app__ + " v" + __version__)
        self.mqtt.on_message_cleaned = self.mqtt_on_message
        self.mqtt.subscribe_to = ['/ipc/rf24mqtt'] + self._actions.keys()
        self.mqtt.logger = self.logger
        self.rf24.logger = self.logger

        self.mqtt.connect()
        if not self.rf24.connect():
            self.stop()

        while True:
            if (self.mqtt.loop() == 0):
                time.sleep(0.1)
            else:
                self.mqtt.connect()


def main():
    """
    The main function; it all begins here
    """

    def resolve_path(path):
        """
        Return an absolute path
        """
        if path[0] == '/':
            return path
        else:
            dirname = os.path.dirname(os.path.realpath(__file__))
            return os.path.join(dirname, path)

    devices = Config(resolve_path('config/Devices.yaml')).get('devices')

    config = Config(resolve_path('config/RF24MQTT.yaml'))
    config['rf24_node_command'] = resolve_path('libs/RF24Node')
    config['devices'] = devices

    handler = logging.StreamHandler()
    handler.setFormatter(
        logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    )

    logger = logging.getLogger()
    logger.setLevel(config.get('general', 'logging_level', logging.INFO))
    logger.addHandler(handler)

    mqtt = MosquittoWrapper.ConfigureNewWrapper(config)
    rf24 = RF24Wrapper.ConfigureNewWrapper(config)
    processor = Processor(devices = devices)

    pid = resolve_path(config.get('general', 'pidfile', '/tmp/RF24MQTT.pid'))
    std_out_err = resolve_path(config.get('general', 'stdout', '/dev/null'))
    dupe_chk_win = config.get('general', 'duplicate_check_window', 1)
    def_pat = config.get('general', 'default_topic_pattern', '/raw/rf24/{id}')
    pub_undef_top = config.get('general', 'publish_undefined_topics', False)

    rf24mqtt = RF24MQTT(resolve_path(pid))
    rf24mqtt.stdout = rf24mqtt.stderr = std_out_err
    rf24mqtt.duplicate_check_window = dupe_chk_win
    rf24mqtt.default_topic_pattern = def_pat
    rf24mqtt.publish_undefined_topics = pub_undef_top
    rf24mqtt.logger = logger
    rf24mqtt.mqtt = mqtt
    rf24mqtt.rf24 = rf24
    rf24mqtt.processor = processor
    rf24mqtt.load(devices)

    if len(sys.argv) == 2:
        kill_rf24 = 'ps aux | grep RF24Node | awk \'{print $2}\' | ' \
                    'sudo xargs kill -9'

        if 'start' == sys.argv[1]:
            rf24mqtt.start()
        elif 'stop' == sys.argv[1]:
            os.system(kill_rf24)
            rf24mqtt.stop()
        elif 'restart' == sys.argv[1]:
            os.system(kill_rf24)
            rf24mqtt.restart()
        else:
            print "Unknown command"
            sys.exit(2)
        sys.exit(0)
    else:
        print "usage: %s start|stop|restart" % sys.argv[0]
        sys.exit(2)

if __name__ == "__main__":
    main()
