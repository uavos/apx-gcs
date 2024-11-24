#pragma once

#include <cinttypes>
#include <sys/types.h>

#include <MandalaMeta.h>
#include <MandalaPack.h>

namespace telemetry {

#pragma pack(1)

static constexpr auto APXTLM_VERSION = 1;

static constexpr auto APXTLM_FTYPE = "apxtlm"; // used by filename and dirname
static constexpr auto APXTLM_MAGIC = "APXTLM"; // first bytes of a file

// file header
struct fhdr_s
{
    static constexpr auto SIZE = 64;

    union {
        uint8_t _raw1[32];
        struct
        {
            char magic[16];          // i.e. "APXTLM"
            uint16_t version;        // format version number
            uint16_t payload_offset; // header size
        };
    };
    union {
        uint8_t _raw2[32];
        struct
        {
            uint64_t timestamp; // file timestamp [ms] since epoch
            int32_t utc_offset; // timestamp UTC offset [s] of local machine
        };
    };
};
static_assert(sizeof(fhdr_s) == fhdr_s::SIZE, "size error");

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
    stop = 0, // dspec==0, stop reading/writing stream (stats are written after this)
    ts,       // <u32> [ms] timestamp update relative to file
    dir,      // direction marker for next dspec (i.e. uplink)
    field,    // <name><u8 N><str1><str2>...<strN> fields defs (name,title,descr,...)
    evtid,    // <name><u8 N><str1><str2>...<strN> event defs (name,value,uid,...)

    // special data types, strings separated by 0
    evt = 8, // <u8 evtid><str1><str2>...<strN> generic event
    jso,     // <name><u32 size><zip[size]> json object, full or diff (i.e. config, mission)
    raw,     // <name><u16 size><data[size]> raw binary data (i.e. serial, vcp)
    zip,     // <name><u32 size><zip[size]> compressed raw binary data
};

static constexpr const auto MIN_ZCOMP = 64;    // minimum size to use zlib compression
static constexpr const auto MAX_STRLEN = 4096; // including null terminator

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
