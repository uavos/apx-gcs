/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Parts of the joystick backend (this file, JoystickBackendLinux.cpp and
 * JoystickBackendMac.cpp) are ported from SDL2 2.30.8 and altered to work
 * with Qt. The origin of each part is noted next to the code.
 *
 * Simple DirectMedia Layer
 * Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include "JoystickBackend.h"

// Platform implementations define JoystickBackend::create():
//  - JoystickBackendLinux.cpp
//  - JoystickBackendMac.cpp

// ---------------------------------------------------------------------------
// SDL_PrivateJoystickAxis (src/joystick/SDL_joystick.c)
//
// Not ported: events filtering by keyboard focus of SDL windows (the plugin
// never had SDL windows) and the exception for virtual joysticks.

void JoystickAxisFilter::update(int v, QVarLengthArray<int, 2> &out)
{
    if (!has_initial_value
        || (!has_second_value && (initial_value <= -32767 || initial_value == 32767)
            && qAbs(v) < (JoystickBackend::AxisMax / 4))) {
        initial_value = v;
        value = v;
        has_initial_value = true;
    } else if (v == value) {
        return;
    } else {
        has_second_value = true;
    }

    if (!sent_initial_value) {
        // Make sure we don't send motion until there's real activity on this axis
        const int MAX_ALLOWED_JITTER = JoystickBackend::AxisMax / 80;
        if (qAbs(v - value) <= MAX_ALLOWED_JITTER)
            return;
        sent_initial_value = true;
        // SDL calls itself here to report the initial position
        has_second_value = true;
        value = initial_value;
        out.append(initial_value);
    }

    value = v;
    out.append(v);
}

// SDL_PrivateJoystickForceRecentering (src/joystick/SDL_joystick.c), the axis part:
// SDL keeps the rest position in 'zero', which always equals initial_value.
void JoystickAxisFilter::recenter(QVarLengthArray<int, 2> &out)
{
    if (has_initial_value)
        update(initial_value, out);
}

// ---------------------------------------------------------------------------
// SDL_CreateJoystickName (src/joystick/SDL_joystick.c)
//
// Not ported: the name by controller type for a device which reports no
// strings (SDL_GetJoystickGameControllerTypeFromVIDPID), such device is named
// by its vendor and product id.

// GuessControllerName (src/joystick/controller_type.c):
// names of well known game controllers, which replace the strings reported
// by hardware. The list is the named entries of arrControllers
// (src/joystick/controller_list.h, Copyright (C) Valve Corporation,
// distributed under the same terms as SDL).
static const char *controllerName(quint16 vendor, quint16 product)
{
    static const struct
    {
        quint32 vidpid;
        const char *name;
    } list[] = {
        {0x25630575, "Retro-bit Controller"},
        {0x045e028e, "Xbox 360 Controller"},
        {0x045e028f, "Xbox 360 Controller"},
        {0x045e0291, "Xbox 360 Wireless Controller"},
        {0x045e02a1, "Xbox 360 Wireless Controller"},
        {0x045e02a9, "Xbox 360 Wireless Controller"},
        {0x045e0719, "Xbox 360 Wireless Controller"},
        {0x0e6f0113, "PDP Xbox 360 Afterglow"},
        {0x0e6f011f, "PDP Xbox 360 Rock Candy"},
        {0x0e6f0125, "PDP INJUSTICE FightStick"},
        {0x0e6f0127, "PDP INJUSTICE FightPad"},
        {0x0e6f0131, "PDP EA Soccer Controller"},
        {0x0e6f0133, "PDP Battlefield 4 Controller"},
        {0x0e6f0143, "PDP MK X Fight Stick"},
        {0x0e6f0147, "PDP Xbox 360 Marvel Controller"},
        {0x0e6f0201, "PDP Xbox 360 Controller"},
        {0x0e6f0213, "PDP Xbox 360 Afterglow"},
        {0x0e6f021f, "PDP Xbox 360 Rock Candy"},
        {0x0e6f0301, "PDP Xbox 360 Controller"},
        {0x0e6f0313, "PDP Xbox 360 Afterglow"},
        {0x0e6f0314, "PDP Xbox 360 Afterglow"},
        {0x0e6f0401, "PDP Xbox 360 Controller"},
        {0x0f0d00db, "HORI Slime Controller"},
        {0x10381430, "SteelSeries Stratus Duo"},
        {0x10381431, "SteelSeries Stratus Duo"},
        {0x1949041a, "Amazon Luna Controller"},
        {0x045e02d1, "Xbox One Controller"},
        {0x045e02dd, "Xbox One Controller"},
        {0x045e02e0, "Xbox One S Controller"},
        {0x045e02e3, "Xbox One Elite Controller"},
        {0x045e02ea, "Xbox One S Controller"},
        {0x045e02fd, "Xbox One S Controller"},
        {0x045e02ff, "Xbox One Controller"},
        {0x045e0b00, "Xbox One Elite 2 Controller"},
        {0x045e0b05, "Xbox One Elite 2 Controller"},
        {0x045e0b0a, "Xbox Adaptive Controller"},
        {0x045e0b0c, "Xbox Adaptive Controller"},
        {0x045e0b12, "Xbox Series X Controller"},
        {0x045e0b13, "Xbox Series X Controller"},
        {0x045e0b20, "Xbox One S Controller"},
        {0x045e0b21, "Xbox Adaptive Controller"},
        {0x045e0b22, "Xbox One Elite 2 Controller"},
        {0x0e6f0139, "PDP Xbox One Afterglow"},
        {0x0e6f013b, "PDP Xbox One Face-Off Controller"},
        {0x0e6f0145, "PDP MK X Fight Pad"},
        {0x0e6f0146, "PDP Xbox One Rock Candy"},
        {0x0e6f015b, "PDP Fallout 4 Vault Boy Controller"},
        {0x0e6f015c, "PDP Xbox One @Play Controller"},
        {0x0e6f015d, "PDP Mirror's Edge Controller"},
        {0x0e6f015f, "PDP Metallic Controller"},
        {0x0e6f0160, "PDP NFL Face-Off Controller"},
        {0x0e6f0161, "PDP Xbox One Camo"},
        {0x0e6f0162, "PDP Xbox One Controller"},
        {0x0e6f0163, "PDP Deliverer of Truth"},
        {0x0e6f0164, "PDP Battlefield 1 Controller"},
        {0x0e6f0165, "PDP Titanfall 2 Controller"},
        {0x0e6f0166, "PDP Mass Effect: Andromeda Controller"},
        {0x0e6f0167, "PDP Halo Wars 2 Face-Off Controller"},
        {0x0e6f0205, "PDP Victrix Pro Fight Stick"},
        {0x0e6f0206, "PDP Mortal Kombat Controller"},
        {0x0e6f0246, "PDP Xbox One Rock Candy"},
        {0x0e6f0261, "PDP Xbox One Camo"},
        {0x0e6f0262, "PDP Xbox One Controller"},
        {0x0e6f02a0, "PDP Xbox One Midnight Blue"},
        {0x0e6f02a1, "PDP Xbox One Verdant Green"},
        {0x0e6f02a2, "PDP Xbox One Crimson Red"},
        {0x0e6f02a3, "PDP Xbox One Arctic White"},
        {0x0e6f02a4, "PDP Xbox One Phantom Black"},
        {0x0e6f02a5, "PDP Xbox One Ghost White"},
        {0x0e6f02a6, "PDP Xbox One Revenant Blue"},
        {0x0e6f02a7, "PDP Xbox One Raven Black"},
        {0x0e6f02a8, "PDP Xbox One Arctic White"},
        {0x0e6f02a9, "PDP Xbox One Midnight Blue"},
        {0x0e6f02aa, "PDP Xbox One Verdant Green"},
        {0x0e6f02ab, "PDP Xbox One Crimson Red"},
        {0x0e6f02ac, "PDP Xbox One Ember Orange"},
        {0x0e6f02ad, "PDP Xbox One Phantom Black"},
        {0x0e6f02ae, "PDP Xbox One Ghost White"},
        {0x0e6f02af, "PDP Xbox One Revenant Blue"},
        {0x0e6f02b0, "PDP Xbox One Raven Black"},
        {0x0e6f02b1, "PDP Xbox One Arctic White"},
        {0x0e6f02b3, "PDP Xbox One Afterglow"},
        {0x0e6f02b5, "PDP Xbox One GAMEware Controller"},
        {0x0e6f02bd, "PDP Xbox One Royal Purple"},
        {0x0e6f02be, "PDP Xbox One Raven Black"},
        {0x0e6f02bf, "PDP Xbox One Midnight Blue"},
        {0x0e6f02c0, "PDP Xbox One Phantom Black"},
        {0x0e6f02c1, "PDP Xbox One Ghost White"},
        {0x0e6f02c2, "PDP Xbox One Revenant Blue"},
        {0x0e6f02c3, "PDP Xbox One Verdant Green"},
        {0x0e6f02c4, "PDP Xbox One Ember Orange"},
        {0x0e6f02c5, "PDP Xbox One Royal Purple"},
        {0x0e6f02c6, "PDP Xbox One Crimson Red"},
        {0x0e6f02c7, "PDP Xbox One Arctic White"},
        {0x0e6f02c8, "PDP Kingdom Hearts Controller"},
        {0x0e6f02c9, "PDP Xbox One Phantasm Red"},
        {0x0e6f02ca, "PDP Xbox One Specter Violet"},
        {0x0e6f02cb, "PDP Xbox One Specter Violet"},
        {0x0e6f02cd, "PDP Xbox One Blu-merang"},
        {0x0e6f02ce, "PDP Xbox One Cranblast"},
        {0x0e6f02cf, "PDP Xbox One Aqualime"},
        {0x0e6f02d5, "PDP Xbox One Red Camo"},
        {0x0e6f0346, "PDP Xbox One RC Gamepad"},
        {0x0e6f0446, "PDP Xbox One RC Gamepad"},
        {0x0e6f02da, "PDP Xbox Series X Afterglow"},
        {0x0e6f02d6, "Victrix Gambit Tournament Controller"},
        {0x0e6f02d9, "PDP Xbox Series X Midnight Blue"},
        {0x20d62001, "PowerA Xbox Series X Controller"},
        {0x20d62002, "PowerA Xbox Series X Controller"},
        {0x20d62003, "PowerA Xbox Series X Controller"},
        {0x20d62004, "PowerA Xbox Series X Controller"},
        {0x20d62005, "PowerA Xbox Series X Controller"},
        {0x20d62006, "PowerA Xbox Series X Controller"},
        {0x20d62009, "PowerA Xbox Series X Controller"},
        {0x20d6200a, "PowerA Xbox Series X Controller"},
        {0x20d6200b, "PowerA Xbox Series X Controller"},
        {0x20d6200c, "PowerA Xbox Series X Controller"},
        {0x20d6200d, "PowerA Xbox Series X Controller"},
        {0x20d6200e, "PowerA Xbox Series X Controller"},
        {0x20d6200f, "PowerA Xbox Series X Controller"},
        {0x20d62011, "PowerA Xbox Series X Controller"},
        {0x20d62012, "PowerA Xbox Series X Controller"},
        {0x20d62015, "PowerA Xbox Series X Controller"},
        {0x20d62016, "PowerA Xbox Series X Controller"},
        {0x20d62017, "PowerA Xbox Series X Controller"},
        {0x20d62018, "PowerA Xbox Series X Controller"},
        {0x20d62019, "PowerA Xbox Series X Controller"},
        {0x20d6201a, "PowerA Xbox Series X Controller"},
        {0x20d64001, "PowerA Fusion Pro 2 Controller"},
        {0x20d64002, "PowerA Spectra Infinity Controller"},
        {0x24c6543a, "PowerA Xbox One Controller"},
    };

    const quint32 vidpid = (static_cast<quint32>(vendor) << 16) | product;
    for (const auto &c : list) {
        if (c.vidpid == vidpid)
            return c.name;
    }
    return nullptr;
}

// SDL_tolower: ASCII letters only
static QChar asciiLower(QChar c)
{
    return (c >= QChar('A') && c <= QChar('Z')) ? QChar(c.unicode() + ('a' - 'A')) : c;
}

QString JoystickBackend::makeName(quint16 vendor,
                                  quint16 product,
                                  const QString &vendorName,
                                  const QString &productName)
{
    static const struct
    {
        const char *prefix;
        const char *replacement;
    } replacements[] = {
        {"ASTRO Gaming", "ASTRO"},
        {"Bensussen Deutsch & Associates,Inc.(BDA)", "BDA"},
        {"Guangzhou Chicken Run Network Technology Co., Ltd.", "GameSir"},
        {"HORI CO.,LTD", "HORI"},
        {"HORI CO.,LTD.", "HORI"},
        {"Mad Catz Inc.", "Mad Catz"},
        {"Nintendo Co., Ltd.", "Nintendo"},
        {"NVIDIA Corporation ", ""},
        {"Performance Designed Products", "PDP"},
        {"QANBA USA, LLC", "Qanba"},
        {"QANBA USA,LLC", "Qanba"},
        {"Unknown ", ""},
    };

    if (const char *custom_name = controllerName(vendor, product))
        return QString(custom_name);

    QString vendor_name = vendorName;
    QString product_name = productName;
    while (vendor_name.startsWith(' '))
        vendor_name.remove(0, 1);
    while (product_name.startsWith(' '))
        product_name.remove(0, 1);

    QString name;
    if (!vendor_name.isEmpty() && !product_name.isEmpty()) {
        name = vendor_name + ' ' + product_name;
    } else if (!product_name.isEmpty()) {
        name = product_name;
    } else if (vendor || product) {
        name = QString("0x%1/0x%2")
                   .arg(vendor, 4, 16, QChar('0'))
                   .arg(product, 4, 16, QChar('0'));
    } else {
        name = "Controller";
    }

    // Trim trailing whitespace
    while (name.endsWith(' '))
        name.chop(1);

    // Compress duplicate spaces
    for (int i = 0; i < name.size() - 1;) {
        if (name.at(i) == ' ' && name.at(i + 1) == ' ')
            name.remove(i, 1);
        else
            ++i;
    }

    // Perform any manufacturer replacements
    for (const auto &r : replacements) {
        const QString prefix(r.prefix);
        if (name.startsWith(prefix, Qt::CaseInsensitive)) {
            name = QString(r.replacement) + name.mid(prefix.size());
            break;
        }
    }

    // Remove duplicate manufacturer or product in the name
    // e.g. Razer Razer Raiju Tournament Edition Wired
    for (int i = 1; i < name.size() - 1; ++i) {
        // PrefixMatch(name, &name[i])
        int matchlen = 0;
        while (i + matchlen < name.size()
               && asciiLower(name.at(matchlen)) == asciiLower(name.at(i + matchlen)))
            matchlen++;

        while (matchlen > 0) {
            if (name.at(matchlen) == ' ' || name.at(matchlen) == '-') {
                name = name.mid(matchlen + 1);
                break;
            }
            --matchlen;
        }
        if (matchlen > 0) {
            // We matched the manufacturer's name and removed it
            break;
        }
    }

    return name;
}

// ---------------------------------------------------------------------------
// SDL_crc16 (src/stdlib/SDL_crc16.c)

static quint16 crc16_for_byte(quint8 r)
{
    quint16 crc = 0;
    for (int i = 0; i < 8; ++i) {
        crc = ((crc ^ r) & 1 ? 0xA001 : 0) ^ crc >> 1;
        r >>= 1;
    }
    return crc;
}

static quint16 crc16(quint16 crc, const QByteArray &data)
{
    for (int i = 0; i < data.size(); ++i) {
        const quint8 b = static_cast<quint8>(data.at(i));
        crc = crc16_for_byte(static_cast<quint8>(crc) ^ b) ^ crc >> 8;
    }
    return crc;
}

// ---------------------------------------------------------------------------
// SDL_CreateJoystickGUID (src/joystick/SDL_joystick.c)
// with driver_signature = 0 and driver_data = 0, as the Linux and IOKit
// drivers of SDL call it.

QString JoystickBackend::makeUid(quint16 bus,
                                 quint16 vendor,
                                 quint16 product,
                                 quint16 version,
                                 const QByteArray &vendorName,
                                 const QByteArray &productName)
{
    quint16 crc = 0;
    if (!vendorName.isEmpty() && !productName.isEmpty()) {
        crc = crc16(crc, vendorName);
        crc = crc16(crc, QByteArray(" "));
        crc = crc16(crc, productName);
    } else {
        crc = crc16(crc, productName);
    }

    QByteArray guid(16, '\0');
    auto put16 = [&guid](int pos, quint16 v) { // little endian
        guid[pos] = static_cast<char>(v & 0xFF);
        guid[pos + 1] = static_cast<char>(v >> 8);
    };

    // We only need 16 bits for each of these; space them out to fill 128.
    put16(0, bus);
    put16(2, crc);
    if (vendor && product) {
        put16(4, vendor);
        put16(8, product);
        put16(12, version);
    } else {
        // SDL_strlcpy(guid + 4, product_name, 12)
        const QByteArray s = productName.left(11);
        for (int i = 0; i < s.size(); ++i)
            guid[4 + i] = s.at(i);
    }
    return QString(guid.toHex().toUpper());
}

// ---------------------------------------------------------------------------
// SDL_ShouldIgnoreJoystick (src/joystick/SDL_joystick.c):
// initial_blacklist_devices and initial_rog_gamepad_mice tables.
//
// Not ported: SDL_ShouldIgnoreGameController (lists configured by SDL hints).

bool JoystickBackend::ignoredDevice(quint16 vendor, quint16 product)
{
    static const quint32 list[] = {
        0x045e009d, // Microsoft Wireless Optical Desktop 2.10, Wireless Desktop - Comfort Edition
        0x045e00b0, // Microsoft Digital Media Pro Keyboard
        0x045e00b4, // Microsoft Digital Media Keyboard 1.0A
        0x045e0730, // Microsoft Digital Media Keyboard 3000
        0x045e0745, // Microsoft 2.4GHz Transceiver v6.0/v8.0, Wireless Mobile Mouse 1000, Wireless Desktop 3000
        0x045e0748, // Microsoft SideWinder(TM) 2.4GHz Transceiver
        0x045e0750, // Microsoft Corp. Wired Keyboard 600
        0x045e0768, // Microsoft Corp. Sidewinder X4 keyboard
        0x045e0773, // Microsoft Corp. Arc Touch Mouse Transceiver
        0x045e07a5, // Microsoft 2.4GHz Transceiver v9.0, Nano Transceiver v2.1, Sculpt Ergonomic Keyboard
        0x045e07b2, // Microsoft Nano Transceiver v1.0, Wireless Keyboard 800
        0x045e0800, // Microsoft Nano Transceiver v2.0

        0x046dc30a, // Logitech, Inc. iTouch Composite keboard

        0x04d9a0df, // Tek Syndicate Mouse (E-Signal USB Gaming Mouse)

        0x056a0010, // Wacom ET-0405 Graphire
        0x056a0011, // Wacom ET-0405A Graphire2 (4x5)
        0x056a0012, // Wacom ET-0507A Graphire2 (5x7)
        0x056a0013, // Wacom CTE-430 Graphire3 (4x5)
        0x056a0014, // Wacom CTE-630 Graphire3 (6x8)
        0x056a0015, // Wacom CTE-440 Graphire4 (4x5)
        0x056a0016, // Wacom CTE-640 Graphire4 (6x8)
        0x056a0017, // Wacom CTE-450 Bamboo Fun (4x5)
        0x056a0018, // Wacom CTE-650 Bamboo Fun 6x8
        0x056a0019, // Wacom CTE-631 Bamboo One
        0x056a00d1, // Wacom Bamboo Pen and Touch CTH-460
        0x056a030e, // Wacom Intuos Pen (S) CTL-480

        0x09da054f, // A4 Tech Co., G7 750 mouse
        0x09da1410, // A4 Tech Co., Ltd Bloody AL9 mouse
        0x09da3043, // A4 Tech Co., Ltd Bloody R8A Gaming Mouse
        0x09da31b5, // A4 Tech Co., Ltd Bloody TL80 Terminator Laser Gaming Mouse
        0x09da3997, // A4 Tech Co., Ltd Bloody RT7 Terminator Wireless
        0x09da3f8b, // A4 Tech Co., Ltd Bloody V8 mouse
        0x09da51f4, // Modecom MC-5006 Keyboard
        0x09da5589, // A4 Tech Co., Ltd Terminator TL9 Laser Gaming Mouse
        0x09da7b22, // A4 Tech Co., Ltd Bloody V5
        0x09da7f2d, // A4 Tech Co., Ltd Bloody R3 mouse
        0x09da8090, // A4 Tech Co., Ltd X-718BK Oscar Optical Gaming Mouse
        0x09da9033, // A4 Tech Co., X7 X-705K
        0x09da9066, // A4 Tech Co., Sharkoon Fireglider Optical
        0x09da9090, // A4 Tech Co., Ltd XL-730K / XL-750BK / XL-755BK Laser Mouse
        0x09da90c0, // A4 Tech Co., Ltd X7 G800V keyboard
        0x09daf012, // A4 Tech Co., Ltd Bloody V7 mouse
        0x09daf32a, // A4 Tech Co., Ltd Bloody B540 keyboard
        0x09daf613, // A4 Tech Co., Ltd Bloody V2 mouse
        0x09daf624, // A4 Tech Co., Ltd Bloody B120 Keyboard

        0x1b1c1b3c, // Corsair Harpoon RGB gaming mouse
        0x1d57ad03, // [T3] 2.4GHz and IR Air Mouse Remote Control
        0x1e7d2e4a, // Roccat Tyon Mouse
        0x20a0422d, // Winkeyless.kr Keyboards
        0x2516001f, // Cooler Master Storm Mizar Mouse
        0x25160028, // Cooler Master Storm Alcor Mouse

        0x04d98008, // OBINLB USB-HID Keyboard (Anne Pro II)
        0x04d98009, // OBINLB USB-HID Keyboard (Anne Pro II)
        0x04d9a292, // OBINLB USB-HID Keyboard (Anne Pro II)
        0x04d9a293, // OBINLB USB-HID Keyboard (Anne Pro II)
        0x15320266, // Razer Huntsman V2 Analog, non-functional DInput device
        0x15320282, // Razer Huntsman Mini Analog, non-functional DInput device
        0x26ce01a2, // ASRock LED Controller
        0x20d60002, // PowerA Enhanced Wireless Controller for Nintendo Switch (charging port only)

        // rog_gamepad_mice
        0x0b051906, // ROG Pugio II
        0x0b051958, // ROG Chakram Core Mouse
        0x0b0518e3, // ROG Chakram (wired) Mouse
        0x0b0518e5, // ROG Chakram (wireless) Mouse
        0x0b051a18, // ROG Chakram X (wired) Mouse
        0x0b051a1a, // ROG Chakram X (wireless) Mouse
        0x0b051a1c, // ROG Chakram X (Bluetooth) Mouse
    };

    const quint32 vidpid = (static_cast<quint32>(vendor) << 16) | product;
    for (quint32 v : list) {
        if (v == vidpid)
            return true;
    }
    return false;
}

#if !(defined(__linux__) && !defined(__ANDROID__)) && !defined(__APPLE__)

JoystickBackend *JoystickBackend::create(QObject *parent)
{
    Q_UNUSED(parent)
    return nullptr;
}

#endif
