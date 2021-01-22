#!/usr/bin/python

import argparse
import os
import subprocess

import simplejson

if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(description='Deploy linux AppImage for APX app.')
    parser.add_argument('--appdata', action='store', required=True, help='APX app metadata json file')
    parser.add_argument('--apprun', action='store', help='AppImage AppRun script')
    args = parser.parse_args()
    with open(args.appdata, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()
    path = os.path.dirname(args.appdata)
    app = json['app']
    app_name = app['name']
    app_path = os.path.abspath(os.path.join(path, app['bundle_path'], '..'))
    packages_path = os.path.join(path, app['packages_path'])
    bundle = os.path.basename(app_path)
    platform = app['platform']
    zsync_link = 'gh-releases-zsync|uavos|apx-releases|latest|APX_Ground_Control-*-linux-x86_64.AppImage.zsync'

    filename = os.path.join(app_name.replace(' ', '_')+'-'+app['version']+'-'+platform+'-'+app['arch']+'.AppImage')
    volume_name = app_name+' ('+app['version']+')'

    print('Deploy image ({})...'.format(filename))

    if args.apprun:
        apprun = os.path.join(app_path, 'AppRun')
        os.remove(apprun)
        subprocess.check_call(['cp', '-af', args.apprun, apprun])

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
