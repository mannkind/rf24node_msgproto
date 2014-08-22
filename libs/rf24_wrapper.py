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
__version__ = "0.1.20131113"
__author__ = "Dustin Brewer"
__contact__ = "mannkind@thenullpointer.net"
__copyright__ = "Copyright (C) 2013 Dustin Brewer"
__license__ = 'GPL v3'

from sh import sudo, kill, Command
import logging

class RF24Wrapper(object):
    """
    Helper class for the rf24_node program.
    """

    def __init__(self):
        self.logger = None
        self.command = None
        self.pid = None

        self.channel = None
        self.palevel = None
        self.datarate = None
        self.node = None
        self.key = None

    def log(self, level, message):
        """
        If a logger is available, log the message with the given level
        """
        if self.logger:
            self.logger.log(level, message)

    def disconnect(self):
        """
        If the pid has been set; kill the task
        """
        if self.pid is not None:
            with sudo: 
                kill('-9', str(self.pid))

        return True

    def connect(self):
        """
        Creates an rf24_node instance
        """
        try:
            self.log(logging.INFO, "Connecting to rf24 w/command %s" % self.command)
            dashv = "-v" if self.logger.getEffectiveLevel() <= logging.INFO else ""
            out = lambda x: self.log(logging.INFO, "RF24Node: %s" % x)
            with sudo:
                result = self.command(dashv, '-c', self.channel, '-p', self.palevel, '-d', self.datarate,
                    '-n', self.node, '-k', self.key, _out = out)

                self.pid = result.pid

        except Exception as e:
            self.log(logging.INFO, 'Ack! Trouble starting RF24 %s' % e)
            return False

        self.log(logging.INFO, "Connected to rf24")
        return True

    @staticmethod
    def ConfigureNewWrapper(config):
        """
        Create an object based on the config
        """
        rf24 = RF24Wrapper()

        rf24.channel = config.get('rf24', 'channel')
        rf24.palevel = config.get('rf24', 'palevel')
        rf24.datarate = config.get('rf24', 'datarate')
        rf24.node = config.get('rf24', 'node')
        rf24.key = config.get('rf24', 'key')
        rf24.command = Command(config.get('rf24_node_command'))

        return rf24
