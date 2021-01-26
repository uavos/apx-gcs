#!/usr/bin/env python3

import argparse
import os

import simplejson
import dmgbuild
# import biplist


def deploy_dmg(path, json):
    app = json['app']
    app_name = app['name']
    app_path = os.path.join(path, app['path']['bundle'])
    bundle = os.path.basename(app_path.rstrip('/'))
    platform = app['platform'].lower()
    arch = app['arch']
    version = app['version']

    filename = os.path.join(app_name.replace(
        ' ', '_')+'-'+version+'-'+platform+'-'+arch+'.dmg')
    volume_name = app_name+' ('+version+')'

    print('Deploy image ({})...'.format(filename))

    settings = {}
    # get icon from app
    # plist_path = os.path.join(app_path, 'Contents', 'Info.plist')
    # print(plist_path)
    # plist = biplist.readPlist(plist_path)
    if 'icon' in app and app['icon']:
        icon_name = app['icon']
        icon_root, icon_ext = os.path.splitext(icon_name)
        if not icon_ext:
            icon_ext = '.icns'
        icon_name = icon_root + icon_ext
        settings['icon'] = os.path.join(
            app_path, 'Contents', 'Resources')+icon_name

    # contents
    files = []
    symlinks = {}
    icon_locations = {}
    # app bundle file
    files.append((app_path, bundle))
    icon_locations[bundle] = (500, 120)
    # Applications link
    symlinks['Applications'] = '/Applications'
    icon_locations['Applications'] = (140, 120)

    settings['files'] = files
    settings['symlinks'] = symlinks
    settings['icon_locations'] = icon_locations

    settings['compression_level'] = 9

    # build image
    dmgbuild.build_dmg(os.path.join(path, filename),
                       volume_name, settings=settings)

    print('Application image created.')


if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(
        description='Deploy DMG apple disk image for APX app.')
    parser.add_argument('--app', action='store',
                        required=True, help='APX app bundle directory')
    parser.add_argument('--meta', action='store',
                        required=True, help='APX app metadata json file')
    args = parser.parse_args()
    with open(args.meta, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()

    path = os.path.abspath(args.app)
    deploy_dmg(path, json)
