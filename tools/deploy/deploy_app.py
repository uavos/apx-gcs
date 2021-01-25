#!/usr/bin/python

import argparse
import os
import subprocess

import simplejson

from deploy_app_libs import deploy_libs
from deploy_app_qt import deploy_qt

if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(
        description='Deploy libs to APX app bundle (qt, etc).')
    parser.add_argument('--app', action='store',
                        required=True, help='APX app bundle absolute path')
    parser.add_argument('--meta', action='store',
                        required=True, help='APX app metadata json file')
    parser.add_argument('--dist', action='store',
                        help='Distribution packages path to add to image')
    parser.add_argument('--sign', action='store',
                        help='Identity to sign bundle')
    args = parser.parse_args()

    with open(args.meta, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()

    path = os.path.dirname(args.app)

    app = json['app']
    platform = app['platform']

    print('Preparing application: '+app['name']+' '+app['version'] +
          ' ' + app['arch'] + ' ' + platform + ' (' + app['path']['bundle'] + ')')

    print('Current directory: {}'.format(os.getcwd()))

    print('')

    deploy_qt(path, json)
    print('')

    deploy_libs(path, json, args.dist)

    print('')
    if args.sign:
        print('Signing app bundle ({})...'.format(args.sign))
        if platform == 'darwin':
            subprocess.check_call([
                'codesign',
                '--deep',
                '--force',
                '-s',
                args.sign,
                os.path.join(path, app['path']['bundle'])
            ])

    print('App bundle prepared.')
