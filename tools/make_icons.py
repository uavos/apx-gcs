#!/usr/bin/python
#
# APX Autopilot project <http://docs.uavos.com>
#
# Copyright (c) 2003-2021, Aliaksei Stratsilatau <sa@uavos.com>
# All rights reserved
#
# This file is part of APX Ground Control.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#


import os
import argparse
import cssutils
import simplejson


# Parse commandline
parser = argparse.ArgumentParser(description='Material Design Icons CSS parser for the APX system.')
parser.add_argument('--src', action='store', required=True, help='source CSS file path')
parser.add_argument('--dest', action='store', required=True, help='destination JSON file path')
args = parser.parse_args()


print("Parsing CSS...")

css = cssutils.parseFile(args.src)

icons = {}


for rule in css:
    if rule.type == rule.STYLE_RULE:
        style = rule.selectorText
        if not style.startswith('.mdi-'):
            continue
        if not style.endswith('::before'):
            continue
        icon = style[5:][:-8]
        # print icon
        for item in rule.style:
            if item.name == 'content':
                s = item.value.strip('\"')
                icons[icon] = s.encode('utf8')

with open(args.dest, 'w') as f:
    f.write(simplejson.dumps(icons, indent=2))

print 'Found {} icons.'.format(len(icons))
