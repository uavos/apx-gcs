#!/usr/bin/env python3

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

    zsync_link = 'gh-releases-zsync|uavos|apx-releases|latest|' + \
        app_name_file+'-*-'+platform+'-'+app_arch+'.AppImage.zsync'

    filename = os.path.join(app_name_file+'-'+version +
                            '-'+platform+'-'+app_arch+'.AppImage')
    volume_name = app_name+' ('+version+')'

    print('Deploy image ({})...'.format(filename))

    if apprun:
        apprun = os.path.join(app_path, 'AppRun')
        os.remove(apprun)
        subprocess.check_call(['cp', '-af', apprun, apprun])

    pargs = [
        'env', 'ARCH='+app['arch'],
        os.path.join(app['qt_bin'], 'appimagetool'),
        '-u', zsync_link,
        # '--comp=xz',
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
