#pragma once

#include <cinttypes>
#include <sys/types.h>

#include "MandalaMetaBase.h"

namespace telemetry {

#pragma pack(1)

// file header
struct fhdr_s
{
    // 32 bytes of file header format UID
    struct magic_s
    {
        char magic[16];   // i.e. APXTLM11
        uint16_t version; // version number (11)

        uint8_t _pad[14];
    } magic;
    static_assert(sizeof(magic_s) == 32, "size error");

    // current version implementation

    struct info_s
    {
        uint64_t time; // file timestamp [ms since epoch] 1970-01-01, Coordinated Universal Time

        uint8_t _pad[32 - 8];
    } info;

    char tags[256 - 32 - 32]; // tags list
};
static_assert(sizeof(fhdr_s) == 256, "size error");

// data format identifiers
enum class dspec_e { // 4 bits
    ext,             // extended uid data (uid=extid_e)

    // unsigned raw
    u8,
    u16,
    u24,
    u32,
    u64,

    // floating point
    f16,
    f32,
    f64,

    // special data types
    null,

    // angle
    a16,
    a32,

    //
    _rsv12,
    _rsv13,
    _rsv14,
    _rsv15,
};

// special non-value data formats
enum class extid_e { // 4 bits
    // core services
    ts = 0, // [ms] u32 timestamp update relative to file
    fields, // [names...,0] strings of used fields sequence
    crc,    // [crc32] counted so far for the data stream

    // events
    evt = 8, // [ts,uplink,name,value,uid,0] event data
    msg,     // [ts,text,uid] message and source node uid
    file,    // [name,json_base64_zip]

};

// data specifier (1 or 2 bytes)
union dspec_s {
    uint8_t _raw8 : 8;
    uint16_t _raw16 : 16;

    struct
    {
        dspec_e dspec : 4;
        bool opt : 1;   // when set to 1 use 8 bits vs 16
        uint vidx : 11; // index of variable in the sequence
    } spec16;

    struct
    {
        dspec_e dspec : 4;
        bool opt : 1;        // must be set to 1
        uint vidx_delta : 3; // index delta [1..8] of variable in the sequence
    } spec8;

    struct
    {
        dspec_e dspec : 4;
        extid_e extid : 4; // special field
    } spec_ext;
};
static_assert(sizeof(dspec_s) == 2, "size error");

#pragma pack()

} // namespace telemetry
