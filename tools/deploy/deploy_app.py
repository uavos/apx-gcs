#!/usr/bin/python

import argparse
import os
import subprocess

import simplejson

from deploy_app_libs import deploy_libs
from deploy_app_qt import deploy_qt

if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(description='Deploy libs to APX app bundle (qt, etc).')
    parser.add_argument('--appdata', action='store', required=True, help='APX app metadata json file')
    parser.add_argument('--dist', action='store', help='Distribution packages path to add to image')
    parser.add_argument('--sign', action='store', help='Identity to sign bundle')
    args = parser.parse_args()
    with open(args.appdata, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()
    path = os.path.dirname(args.appdata)
    app = json['app']
    platform = app['platform']

    print('Preparing: '+app['name']+' '+app['version'] +
          ' '+app['arch']+' '+platform+' ('+app['bundle_path']+')')

    print('')

    deploy_qt(path, json)
    print('')
    deploy_libs(path, json, args.dist)

    print('')
    if args.sign:
        print('Signing app bundle ({})...'.format(args.sign))
        if platform == 'macos':
            subprocess.check_call([
                'codesign',
                '--deep',
                '--force',
                '-s',
                args.sign,
                os.path.join(path, app['bundle_path'])
            ])

    print('App bundle prepared.')
