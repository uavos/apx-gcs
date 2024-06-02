#pragma once

#include <cinttypes>
#include <sys/types.h>

#include "MandalaMetaBase.h"

namespace telemetry {

#pragma pack(1)

static constexpr auto APXTLM_VERSION = 1;

static constexpr auto APXTLM_FTYPE = "telemetry";
static constexpr auto APXTLM_MAGIC = "APXTLM";

// file header
struct fhdr_s
{
    // 32 bytes of file header format UID
    union magic_s {
        uint8_t _raw[32];
        struct
        {
            char magic[16];   // i.e. APXTLM
            uint16_t version; // version number
            uint16_t hsize;   // header size
        };
    } magic;
    static_assert(sizeof(magic_s) == 32, "size error");

    // current version implementation

    union info_s {
        uint8_t _raw[64];
        struct
        {
            uint64_t time; // file timestamp [ms since epoch] 1970-01-01, Coordinated Universal Time
            int32_t utc_offset; // timestamp UTC offset of local machine

            // the following data can be rewritten after file is recorded
            // used for consistency check and fast stats access
            uint64_t size; // payload size [bytes]
            uint32_t tmax; // total time [ms] or max timestamp
            struct
            {
                uint32_t total;    // total records count
                uint32_t downlink; // downlink records count
                uint32_t uplink;   // uplink records count
                uint32_t events;   // events records count
                uint16_t fields;   // fields count
                uint16_t meta;     // meta objects count
            } cnt;

            // the following data is ponters to the meta records when available
            // relative to the start of the file (must be non-zero)
            struct
            {
                uint32_t vehicle;
                uint32_t mission;
                uint32_t meta;
            } offset;
        };
    } info;
    static_assert(sizeof(info_s) == 64, "size error");

    // tags: callsign,vehicle_uid,[anything else to help identify the case]
    // tags are always <name:value> pairs null terminated strings
    char tags[1024 - sizeof(magic_s) - sizeof(info_s)];
};
static_assert(sizeof(fhdr_s) == 1024, "size error");

// data format identifiers
enum class dspec_e : uint8_t { // 4 bits
    ext,                       // extended uid data (uid=extid_e)

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

    // angle [deg]
    a16,
    a32,

    //
    _rsv12,
    _rsv13,
    _rsv14,
    _rsv15,
};

static constexpr const char *dspec_names[] = {
    "ext",
    "u8",
    "u16",
    "u24",
    "u32",
    "u64",
    "f16",
    "f32",
    "f64",
    "null",
    "a16",
    "a32",
    "_rsv12",
    "_rsv13",
    "_rsv14",
    "_rsv15",
};

// special non-value data formats
enum class extid_e { // 4 bits (part of dspec)
    // core services
    ts = 0, // [ms] u32 timestamp update relative to file
    uplink, // [dspec,data] uplink data (marks next dspec)
    field,  // [name,title,units] strings of used fields sequence
    crc,    // [crc32] counted so far for the data stream (excl header)

    // special data types, strings separated by 0 and list terminated by another 0
    evt = 8, // [name,value,uid,0] generic event (conf update)
    msg,     // [text,subsystem,0] text message
    meta,    // [name,size(32),meta_zip(...)] (nodes,mission)
    mupd,    // [name,size(32),meta_zip(...)] meta data patch (json diff)
    raw,     // [id(16),size(16),data(...)] raw data (serial vcp)
};

// data specifier (1 or 2 bytes)
union dspec_s {
    uint8_t _raw8 : 8;
    uint16_t _raw16 : 16;

    struct
    {
        dspec_e dspec : 4;
        bool opt8 : 1;  // when set to 1 use 8 bits vs 16
        uint vidx : 11; // index of variable in the sequence
    } spec16;

    struct
    {
        dspec_e dspec : 4;
        bool opt8 : 1;       // must be set to 1
        uint vidx_delta : 3; // index delta [1..8] of variable in the sequence
        uint _rsv : 8;
    } spec8;

    struct
    {
        dspec_e dspec : 4;
        extid_e extid : 4; // special field
        uint _rsv : 8;
    } spec_ext;
};
static_assert(sizeof(dspec_s) == 2, "size error");

#pragma pack()

} // namespace telemetry
