//=============================================================
#ifndef _drv_rec_H
#define _drv_rec_H
#include "drv_base.h"
#include "time_ms.h"
#include "Mandala.h"
//==============================================================================
class _drv_rec : public _drv_base, public Dispatcher<evt_var_changed>::Listener
{
public:
    _drv_rec(Bus *b, const char *fname)
        : _drv_base(b)
        , disabled(false)
    {
        fd = fopen(fname, "w");
        if (!fd) {
            dmsg("Error opening rec file: %s\n", fname);
        }
        memset(&rec, 0, sizeof(rec));
        bus->BIND(evt_var_changed);
    }

private:
    FILE *fd;
    bool disabled;
    typedef struct
    {
        uint32_t timestamp;     //time [ms]
        float roll, pitch, yaw; //attitude [deg]
        float ax, ay, az;       //accelerations [m/s2]
        float p, q, r;          //gyro [deg/s]
        float hx, hy, hz;       //magnetometer [a.u.]
        float altps;            //static pressure altitude [m]
        float airspeed;         //airspeed CAS [m/s]
        float lat, lon, hmsl;   //GPS coordinates[deg,deg,m]
        float vn, ve, vd;       //GPS speeds [m/s]
    } __attribute__((packed)) _rec;
    _rec rec;
    //driver override
    void OnEvent(evt_var_changed &)
    {
        if (disabled)
            return;
        extern Mandala var;
        if (!fd)
            return;
        uint8_t var_idx = Dispatcher<evt_var_changed>::Listener::event_flags;
        if (var_idx == idx_gps_pos) {
            rec.lat = var.gps_pos[0];
            rec.lon = var.gps_pos[1];
            rec.hmsl = var.gps_pos[2];
        } else if (var_idx == idx_gps_vel) {
            rec.vn = var.gps_vel[0];
            rec.ve = var.gps_vel[1];
            rec.vd = var.gps_vel[2];
        } else if (var_idx == idx_acc) {
            rec.ax = var.acc[0];
            rec.ay = var.acc[1];
            rec.az = var.acc[2];
        } else if (var_idx == idx_gyro) {
            rec.timestamp = time;
            rec.roll = var.theta[0];
            rec.pitch = var.theta[1];
            rec.yaw = var.theta[2];
            rec.p = var.gyro[0];
            rec.q = var.gyro[1];
            rec.r = var.gyro[2];
            rec.hx = var.mag[0];
            rec.hy = var.mag[1];
            rec.hz = var.mag[2];
            rec.altps = var.altps;
            rec.airspeed = var.airspeed;
            disabled |= fwrite(&(rec), sizeof(_rec), 1, fd) != 1;
            if (disabled)
                dmsg("Error writing rec file.\n");
        }
    }
};
//==============================================================================
#endif
