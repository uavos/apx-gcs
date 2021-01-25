#!/usr/bin/python

import argparse
import os
import subprocess

import simplejson

import deploy_utils as utils


def deploy_qt(path, json):
    app = json['app']
    platform = app['platform'].lower()
    print('Deploy Qt...')

    # if os.path.exists(os.path.join(path, app['path']['data'], 'qt.conf')) or os.path.exists(os.path.join(path, app['path']['bin'], 'qt.conf')):
    #     print('Qt already deployed.')
    #     return

    deploy_tool = {
        'darwin': 'macdeployqt',
        'linux': 'linuxdeployqt',
        'windows': 'windeployqt'
    }
    deploy_name = {
        'darwin': app['path']['bundle'],
        'linux': os.path.join(app['path']['bundle'], "share/applications", app['name']+".desktop"),
        'windows': app['path']['bundle']
    }
    opts = list()
    # general opts
    opts.append(os.path.join(app['path']['qt'], 'bin', deploy_tool[platform]))
    opts.append(os.path.join(path, deploy_name[platform]))
    opts.append(
        '-libpath='+os.path.abspath(os.path.join(path, app['path']['libs'])))
    opts.append('-qmldir='+app['path']['src'])

    # opts.append('-verbose=3')

    # platform dependednt
    if 'plugins' in json:
        plugins_path = os.path.join(path, app['path']['plugins'])
        for p in json['plugins']:
            if platform == 'darwin':
                if os.path.exists(os.path.join(plugins_path, p+'.bundle')):
                    p = p + '.bundle/Contents/MacOS/' + p
                else:
                    p = p + '.dylib'
            p = os.path.join(plugins_path, p)
            opts.append('-executable='+p)

    if 'executables' in json:
        for p in json['executables']:
            p = os.path.join(path, app['path']['bin'], p)
            opts.append('-executable='+p)

    # if 'libs' in json:
    #     for p in json['libs']:
    #         p = os.path.join(path, app['path']['libs'], p)
    #         opts.append('-executable='+p)

    if 'qtplugins' in json and platform == 'linux':
        if len(json['qtplugins']) > 0:
            opts.append('-extra-plugins='+','.join(json['qtplugins']))

    if platform == 'linux':
        opts.append('-unsupported-allow-new-glibc')
        opts.append('-no-copy-copyright-files')
        opts.append('-no-translations')
        opts.append('-qmake='+os.path.join(app['path']['qt'], 'qmake'))
        # opts.append('-bundle-non-qt-libs')
        # opts.append('-exclude-libs=')
        # opts.append('-verbose=2')
    elif platform == 'darwin':
        opts.append('-appstore-compliant')
        # opts.append('-libpath=/Library/Frameworks')

    # print('{}'.format(opts))
    # call tool
    subprocess.check_call(opts)

    # clean up
    print('Removing unnecessary components...')
    if platform == 'linux':
        app_path = os.path.abspath(os.path.join(path, app['path']['bundle']))
        utils.remove_all(
            app_path, ['designer', 'Fusion', 'Imagine', 'Universal'])
        utils.remove_all(os.path.join(app_path, 'qml', 'QtQuick'), ['*.qml'])
    elif platform == 'darwin':
        qml_path = os.path.abspath(os.path.join(
            path, app['path']['data'], 'qml'))
        utils.remove_all(
            qml_path, ['designer', 'Fusion', 'Imagine', 'Universal'])
        utils.remove_all(os.path.join(qml_path, 'QtQuick'), ['*.qml'])

    utils.remove_all(os.path.abspath(
        os.path.join(path, app['path']['libs'])), ['*.prl'])

    print('Qt successfully deployed.')


if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(
        description='Deploy Qt libs to APX app bundle with \'XXXdeployqt\' tool.')
    parser.add_argument('--appdata', action='store',
                        required=True, help='APX app metadata json file')
    args = parser.parse_args()
    with open(args.appdata, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()
        deploy_qt(os.path.dirname(args.appdata), json)
