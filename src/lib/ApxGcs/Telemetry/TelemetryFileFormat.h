#pragma once

#include <cinttypes>
#include <sys/types.h>

#include "MandalaMetaBase.h"

namespace telemetry {

#pragma pack(1)

static constexpr auto APXTLM_VERSION = 1;

static constexpr auto APXTLM_FTYPE = "telemetry";
static constexpr auto APXTLM_MAGIC = "APXTLM";
static constexpr auto APXTLM_META = "meta"; // tag to mark file stats meta data

// file header
struct fhdr_s
{
    static constexpr auto SIZE = 1024;
    static constexpr auto SIZE_MIN = 1024;

    // file header format UID
    union {
        uint8_t _raw1[64];
        struct
        {
            char magic[16];          // i.e. "APXTLM"
            uint16_t version;        // version number
            uint16_t payload_offset; // header size, i.e. sizeof(fhdr_s)
        };
    };

    // const data, written at the time of file creation
    union {
        uint8_t _raw2[64];
        struct
        {
            uint64_t timestamp; // file timestamp [ms since epoch]
            int32_t utc_offset; // timestamp UTC offset of local machine
            uint8_t _rsv1[4];

            struct
            {
                uint8_t version[3]; // recorder software version: X.Y.Z
                uint8_t _rsv2[1];

                uint32_t hash; // git hash
            } sw;
        };
    };

    // file header info structure, updated after file is recorded
    union info_s {
        static constexpr auto OFFSET = 128;
        static constexpr auto SIZE = 512 - OFFSET;
        uint8_t _raw[SIZE];
        struct
        {
            // file header SHA1 (leftmost 8 bytes)
            // start from end of this field, end at payload start
            uint64_t hcrc;

            union {
                uint64_t _raw;
                struct
                {
                    bool parsed : 1;    // file parsed and meta data collected
                    bool corrupted : 1; // ever found file corrupted
                    bool edited : 1;    // file edited (splitted or merged)
                };
            } flags;

            uint64_t parse_ts; // file parse timestamp [ms since epoch]

            // used for consistency check and quick stats access
            uint64_t payload_size; // payload size [bytes], excluding metadata at tail
            uint64_t meta_offset;  // meta data offset [bytes] from the start of the file

            uint32_t tmax; // total time [ms] or max timestamp
            uint8_t _rsv1[4 + 8];

            // @48

            uint8_t sha1[20]; // file payload SHA1 hash, excluding metadata at tail
            uint8_t _rsv2[4 + 16];

            // @96

            uint8_t _padding;
        };
    } info;
    static_assert(sizeof(info) == info_s::SIZE, "size error");

    // tags: callsign,vehicle_uid,[anything else to help identify the case]
    // tags are always <name:value> pairs null terminated strings
    static constexpr auto TAGS_SIZE = SIZE - info_s::OFFSET - info_s::SIZE;
    char tags[TAGS_SIZE];
};
static_assert(sizeof(fhdr_s) == fhdr_s::SIZE, "size error");

// tests
namespace _tests {
static constexpr auto info_extra = sizeof(fhdr_s::info) - offsetof(fhdr_s::info_s, _padding);
static constexpr auto padding_offset = offsetof(fhdr_s::info_s, _padding);
static constexpr auto tags_offset = offsetof(fhdr_s, tags);

static_assert(padding_offset == 96, "size error");
static_assert(tags_offset == 512, "size error");
} // namespace _tests

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
enum class extid_e : uint8_t { // 4 bits (part of dspec)
    // core services
    stop = 0, // [dspec==0] stop reading/writing stream
    ts,       // [ms] u32 timestamp update relative to file
    uplink,   // [dspec,data] uplink data (marks next dspec)
    field,    // [name,title,units] strings of used fields sequence
    // crc,      // [sha1-32] counted so far for the data stream (excl header)

    // special data types, strings separated by 0 and list terminated by another 0
    evt = 8, // [name,value,uid,0] generic event (conf update)
    msg,     // [text,subsystem,0] text message
    meta,    // [name,size(32),meta_zip(...)] json full or diff (nodes,mission)
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
