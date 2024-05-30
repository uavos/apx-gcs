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

import argparse
import os
import subprocess

import simplejson


def deploy_appimage(path, json, apprun):
    app = json['app']
    app_name = app['name']
    app_path = os.path.abspath(os.path.join(path, app['path']['bundle'], '..'))

    platform = app['platform'].lower()
    app_arch = app['arch']
    version = app['version']

    packages_path = path

    app_name_file = app_name.replace(' ', '_')

    zsync_link = 'gh-releases-zsync|uavos|apx-gcs|latest|' + \
        app_name_file+'-*-'+platform+'-'+app_arch+'.AppImage.zsync'

    filename = os.path.join(app_name_file+'-'+version +
                            '-'+platform+'-'+app_arch+'.AppImage')
    volume_name = app_name+' ('+version+')'

    print('Deploy image ({})...'.format(filename))

    # remove old images
    for f in os.listdir(packages_path):
        if f.endswith('.AppImage'):
            os.remove(os.path.join(packages_path, f))
    

    if apprun:
        dest = os.path.join(app_path, 'AppRun')
        if os.path.exists(dest):
            os.remove(dest)
        subprocess.check_call(['cp', '-af', apprun, dest])
        subprocess.check_call(['chmod', '+x', dest])

    pargs = [
        'env', 'ARCH='+app_arch,
        'appimagetool',
        # '-u', zsync_link,
        # '--no-appstream',
        '--comp=xz',
        # '--verbose',
        app_path,
        os.path.join(packages_path, filename)
    ]
    subprocess.check_call(pargs, cwd=packages_path)

    # out = deploy_utils.run_and_get_output(pargs)
    # if out.retcode != 0:
    #     print(out.stderr)
    #     exit(out.retcode)
    print('Application image created.')


if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(
        description='Deploy linux AppImage for APX app.')
    parser.add_argument('--app', action='store',
                        required=True, help='APX app bundle directory')
    parser.add_argument('--meta', action='store',
                        required=True, help='APX app metadata json file')
    parser.add_argument('--apprun', action='store',
                        help='AppImage AppRun script')
    args = parser.parse_args()
    with open(args.meta, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()

    path = os.path.abspath(args.app)
    apprun = args.apprun if args.apprun else None

    deploy_appimage(path, json, apprun)
