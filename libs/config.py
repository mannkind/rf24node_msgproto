#! /usr/bin/python
# -*- coding: utf-8 -*-
# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

#   Copyright (C) 2012 by Xose Pérez, Dustin Brewer
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

__author__ = "Xose Pérez, Dustin Brewer"
__contact__ = "xose.perez@gmail.com, mannkind@thenullpointer.net"
__copyright__ = "Copyright (C) 2012-2013 Xose Pérez, Dustin Brewer"
__license__ = 'GPL v3'

import yaml

class Config(object):
    """
    Simple YAML configuration parser
    """

    filename = None
    config = None
    writeback = None

    def __init__(self, filename, writeback=False):
        """
        Constructor, parses and stores the configuration
        """
        self.filename = filename
        self.writeback = writeback
        handler = file(filename, 'r')
        self.config = yaml.load(handler)
        handler.close()

    def get(self, section, key=None, default=None):
        """
        Retrieves a given section/key combination,
        if not existent it return a default value
        """
        try:
            if key is None:
                return self.config[section]
            else:
                return self.config[section][key]
        except:
            return default

    def set(self, section, key=None, value=None):
        """
        Set a given section/key combination,
        if not existent, create it
        """
        try:
            if section not in self.config:
                self.config[section] = None if key is None else {}

            if key is None:
                self.config[section] = value
            else:
                self.config[section][key] = value

            if self.writeback:
                yaml.dump(self.config, open(self.filename, 'w'))

            return True
        except:
            return False

    """
    Mimic the dict methods
    """
    def __len__(self):
        return len(self.config)

    def __getitem__(self, key):
        return self.config[key]

    def __setitem__(self, key, value):
        self.config[key] = value

        if self.writeback:
            yaml.dump(self.config, open(self.filename, 'w'))

    def __delitem__(self, key):
        del self.config[key]

    def __iter__(self):
        return self.config.iterkeys()

    def __contains__(self, item):
        return item in self.config





