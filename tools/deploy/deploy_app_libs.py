#!/usr/bin/python

import argparse
import os
import subprocess
from fnmatch import fnmatch

import simplejson

import deploy_utils as utils
import osxrelocator


def strip_framework_gstreamer(src, dst):
    name = os.path.split(dst)[1]
    gst_bin = os.path.join(dst, 'Versions', os.readlink(os.path.join(dst, 'Versions/Current')))

    utils.remove(os.path.join(gst_bin, 'bin'))
    utils.remove(os.path.join(gst_bin, 'etc'))
    utils.remove(os.path.join(gst_bin, 'share'))
    utils.remove(os.path.join(gst_bin, 'include'))
    utils.remove(os.path.join(gst_bin, 'lib/gst-validate-launcher'))
    utils.remove_all(dst, ['static', '*.pc', '*.sh', 'girepository-*'])
    utils.clean(dst)
    # relocate libs
    rel = '@executable_path/../Frameworks/'+name+'/'
    print("Relocating GStreamer...")
    osxrelocator.OSXRelocator(gst_bin, src, rel, recursive=True).relocate()
    os.symlink('../../../../../Frameworks', os.path.join(gst_bin, 'libexec', 'Frameworks'))


def copy_framework(src, dst, arch):
    name = os.path.split(src)[1]
    dst = os.path.join(dst, name)
    # print('Installing '+name)
    if os.path.exists(dst):
        return
    utils.remove(dst)
    subprocess.call(['ditto', '-v', '--arch', arch, src, dst])
    # copy_tree(fw, dst,  preserve_symlinks=True, verbose=1)
    utils.strip_framework(dst)
    if 'gstreamer' in name.lower():
        strip_framework_gstreamer(src, dst)


# linux support only
def copy_libs_flat(src, dst, patterns, links=False):
    for root, dirs, files in os.walk(src):
        for f in files:
            if not links and os.path.islink(os.path.join(root, f)):
                continue
            for rd in patterns:
                if not fnmatch(f, rd):
                    continue
                t = f
                if not links:
                    fl = f.split('.')
                    ext = fl.index('so')
                    if ext < 0:
                        print('Unable to copy lib: '+f)
                        continue
                    if len(fl) > (ext+1):
                        ext = ext+1
                    t = '.'.join(fl[0:ext+1])
                subprocess.check_call(['cp', '-af', os.path.join(root, f), os.path.join(dst, t)])


def deploy_libs(path, json, dist):
    app = json['app']
    print('Deploy libraries...')

    arch = app['arch']
    platform = app['platform']

    if 'frameworks' in json:
        frameworks = list(json['frameworks'])
        print("Frameworks: {}".format(len(frameworks)))
    else:
        frameworks = []

    if 'libs' in json:
        libs = list(json['libs'])
        print("Libs: {}".format(len(libs)))
    else:
        libs = []

    # install frameworks
    library_path = os.path.join(path, app['library_path'])

    for fw in frameworks:
        if not fw.startswith('/Library/Frameworks'):
            continue
        copy_framework(fw, library_path, arch)

    # # install dist packages
    if dist:
        if not os.path.exists(dist) or not os.listdir(dist):
            print('Packages skipped: '+dist)
        else:
            print('Deploy packages...')
            dist_tmp = os.path.join(path, 'dist')
            utils.remove(dist_tmp)
            os.mkdir(dist_tmp)
            for f in os.listdir(dist):
                if os.path.splitext(f)[1] == '.deb':
                    print('Extracting: '+f)
                    subprocess.call(['dpkg', '-x', os.path.join(dist, f), dist_tmp])
            copy_libs_flat(dist_tmp, os.path.join(path, app['bundle_path'], 'lib'), ['*.so', '*.so.*'], True)
            # app_path = os.path.abspath(os.path.join(path, app['bundle_path']))
            # subprocess.call(['cp', '-ar', os.path.join(dist_tmp, 'usr', 'lib'), app_path])
            # subprocess.call(['cp', '-ar', os.path.join(dist_tmp, 'lib'), app_path])
            # subprocess.call(['find', os.path.join(app_path, 'lib', 'x86_64-linux-gnu'), '-exec', 'mv', '-f', '{}', os.path.join(app_path, 'lib/'), ';'])
            # subprocess.call(['rm', '-rf', os.path.join(app_path, 'lib', 'x86_64-linux-gnu')])
            # clean up
            utils.remove(dist_tmp)
            # remove_all(os.path.join(app_path, 'usr', 'lib'), [
            #     'girepository-*',
            #     'glib-*',
            # ])
            # remove(os.path.join(app_path, 'lib'))
            # remove(os.path.join(app_path, 'var'))

    if platform == 'macos':
        print('Relocating executables...')
        osxrelocator.OSXRelocator(os.path.join(path, app['plugin_path']), '/Library/Frameworks', '@executable_path/../Frameworks', recursive=True).relocate()
        osxrelocator.OSXRelocator(os.path.join(path, app['bin_path']), '/Library/Frameworks', '@executable_path/../Frameworks', recursive=True).relocate()


if __name__ == "__main__":
    # Parse commandline
    parser = argparse.ArgumentParser(description='Search and copy libs to APX app bundle.')
    parser.add_argument('--appdata', action='store', required=True, help='APX app metadata json file')
    parser.add_argument('--dist', action='store', help='Distribution packages path to add to image')
    args = parser.parse_args()
    with open(args.appdata, 'r') as f:
        json = simplejson.loads(str(f.read()))
        f.close()
        deploy_libs(os.path.dirname(args.appdata), json, args.dist)
