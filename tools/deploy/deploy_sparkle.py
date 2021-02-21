#!/usr/bin/python
import argparse
import os
import xml.etree.ElementTree as ElementTree

from subprocess import check_call
from urllib import unquote

# Parse commandline
parser = argparse.ArgumentParser(
    description='Sparkle appcast generator for the APX system.')
parser.add_argument('--assets', required=True, action='store',
                    help='Assets folder path with DMG file')
parser.add_argument('--out', default='appcast.xml', action='store',
                    help='appcast.xml output file')
parser.add_argument('--key', action='store',
                    help='Sparkle EdDSA key')
parser.add_argument('--repo', default='uavos/apx-gcs', action='store',
                    help='releases GitHub repository')
args = parser.parse_args()

#########################
assets = args.assets
out = args.out
repo = args.repo


# generate appcast.xml
print('Generating appcast.xml...')
if os.path.exists(out):
    os.remove(out)

appcast = os.path.join(assets, 'appcast.xml')

app = list()
app.append('generate_appcast')
if args.key:
    app.append('-s')
    app.append(args.key)
app.append('-o')
app.append(appcast)
app.append(assets)
check_call(app)

# fix appcast.xml
ns = 'http://www.andymatuschak.org/xml-namespaces/sparkle'
ElementTree.register_namespace('sparkle', ns)
xml = ElementTree.parse(appcast)
for item in xml.getroot().iter('item'):
    e = item.find('enclosure')
    url = e.attrib['url']
    ver = e.attrib['{' + ns + '}version']
    dl_url = 'https://github.com/{1}/releases/download/release-{0}'.format(
        ver, repo)
    e.attrib['url'] = dl_url + url[url.rfind('/'):]
    e2 = ElementTree.SubElement(item, 'sparkle:releaseNotesLink')
    ruser, rname = repo.split('/')
    e2.text = 'https://{0}.github.io/{1}/docs/releases/release-{2}.html'.format(
        ruser, rname, ver)

    deltas = item.find('{' + ns + '}deltas')
    if deltas is None:
        continue
    for e in deltas.iter('enclosure'):
        url = e.attrib['url']
        ver = e.attrib['{' + ns + '}version']
        ver_from = e.attrib['{' + ns + '}deltaFrom']
        dl_url = 'https://github.com/{1}/releases/download/release-{0}'.format(
            ver, repo)
        file_name = unquote(url[url.rfind('/') + 1:])
        file_rename = 'update_dmg-{0}-{1}.delta'.format(ver, ver_from)
        e.attrib['url'] = dl_url + '/' + file_rename
        if os.path.exists(os.path.join(assets, file_name)):
            os.rename(os.path.join(assets, file_name),
                      os.path.join(assets, file_rename))
# print xml
xml.write(out, xml_declaration=True)
