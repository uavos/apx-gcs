#!/usr/bin/python

import argparse
import os
import subprocess

import simplejson

import deploy_utils as utils


def deploy_qt(path, json):
    app = json['app']
    platform = app['platform']
    print('Deploy Qt...')

    if os.path.exists(os.path.join(path, app['data_path'], 'qt.conf')) or os.path.exists(os.path.join(path, app['bin_path'], 'qt.conf')):
        print('Qt already deployed.')
        return

    deploy_tool = {
        'macos': 'macdeployqt',
        'linux': 'linuxdeployqt',
        'windows': 'windeployqt'
    }
    deploy_name = {
        'macos': app['bundle_path'],
        'linux': os.path.join(app['bundle_path'], "share/applications", app['name']+".desktop"),
        'windows': app['bundle_path']
    }
    opts = list()
    # general opts
    opts.append(os.path.join(app['qt_bin'], deploy_tool[platform]))
    opts.append(os.path.join(path, deploy_name[platform]))
    opts.append('-qmldir='+app['src'])

    # platform dependednt
    if 'plugins' in json:
        for p in json['plugins']:
            p = os.path.join(path, app['plugin_path'], p)
            opts.append('-executable='+p)

    if 'executables' in json:
        for p in json['executables']:
            p = os.path.join(path, app['bin_path'], p)
            opts.append('-executable='+p)

    if 'qtplugins' in json and platform == 'linux':
        if len(json['qtplugins']) > 0:
            opts.append('-extra-plugins='+','.join(json['qtplugins']))

    if platform == 'linux':
        opts.append('-unsupported-allow-new-glibc')
        opts.append('-no-copy-copyright-files')
        opts.append('-no-translations')
        opts.append('-qmake='+os.path.join(app['qt_bin'], 'qmake'))
        # opts.append('-bundle-non-qt-libs')
        # opts.append('-exclude-libs=')
        # opts.append('-verbose=2')
    elif platform == 'macos':
        opts.append('-appstore-compliant')
        # opts.append('-libpath=/Library/Frameworks')

    # call tool
    subprocess.check_call(opts)

    # clean up
    print('Clean...')
    if platform == 'linux':
        app_path = os.path.abspath(os.path.join(path, app['bundle_path']))
        utils.remove_all(app_path, ['designer', 'Fusion', 'Imagine', 'Universal'])
        utils.remove_all(os.path.join(app_path, 'qml', 'QtQuick'), ['*.qml'])
    elif platform == 'macos':
        qml_path = os.path.abspath(os.path.join(path, app['data_path'], 'qml'))
        utils.remove_all(qml_path, ['designer', 'Fusion', 'Imagine', 'Universal'])
        utils.remove_all(os.path.join(qml_path, 'QtQuick'), ['*.qml'])

    print('Qt successfully deployed.')


if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(description='Deploy Qt libs to APX app bundle with \'XXXdeployqt\' tool.')
    parser.add_argument('--appdata', action='store', required=True, help='APX app metadata json file')
    args = parser.parse_args()
    with open(args.appdata, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()
        deploy_qt(os.path.dirname(args.appdata), json)
