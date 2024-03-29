#!/usr/bin/env python3
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
import shutil
from collections import namedtuple
from fnmatch import fnmatch
from subprocess import PIPE, Popen


def run_and_get_output(popen_args):
    """Run process and get all output"""
    process_output = namedtuple(
        'ProcessOutput', ['stdout', 'stderr', 'retcode'])
    try:
        # GlobalConfig.logger.debug('run_and_get_output({0})'.format(repr(popen_args)))

        proc = Popen(popen_args, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        stdout, stderr = proc.communicate(b'')
        proc_out = process_output(stdout, stderr, proc.returncode)

        # GlobalConfig.logger.debug('\tprocess_output: {0}'.format(proc_out))
        return proc_out
    except Exception as exc:
        # GlobalConfig.logger.error('\texception: {0}'.format(exc))
        return process_output('', exc.message, -1)


def sh_escape(s):
    return s.replace("(", "\\(").replace(")", "\\)").replace(" ", "\\ ")


def copytree(src, dst, symlinks=False, ignore=None):
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)


def remove(path):
    # print("rm: {}".format(path))
    if os.path.islink(path):
        os.unlink(path)
        return
    if not os.path.exists(path):
        # print 'missing rm: '+path
        return
    if os.path.isfile(path):
        os.remove(path)
    elif os.path.isdir(path):
        shutil.rmtree(path)


def clean_links(path):
    # clean links
    cnt = 0
    for root, dirs, files in os.walk(path):
        flist = list()
        if dirs:
            flist.extend(dirs)
        if files:
            flist.extend(files)
        for f in flist:
            p = os.path.join(root, f)
            if os.path.islink(p):
                fp = os.path.join(root, os.readlink(p))
                if not os.path.exists(fp):
                    cnt = cnt+1
                    remove(p)
    return cnt


def clean_dirs(path):
    cnt = 0
    # clean empty dirs
    for root, dirs, files in os.walk(path):
        for d in dirs:
            p = os.path.join(root, d)
            if os.path.islink(p):
                continue
            if not os.listdir(p):
                cnt = cnt+1
                remove(p)
    return cnt


def clean(path):
    cnt = 1
    while cnt > 0:
        # print('clean... {}'.format(cnt))
        cnt = clean_dirs(path)
        cnt = cnt + clean_links(path)


def remove_all(path, patterns=[]):
    for root, dirs, files in os.walk(path):
        flist = []
        if dirs:
            flist.extend(dirs)
        if files:
            flist.extend(files)
        for f in flist:
            for rd in patterns:
                if(fnmatch(f, rd)):
                    remove(os.path.join(root, f))
    clean(path)


def strip_framework(path):
    remove_all(path, [
        'Headers', 'PrivateHeaders',
        '*.h',
        '*.a', '*.la', '.DS_Store'])
