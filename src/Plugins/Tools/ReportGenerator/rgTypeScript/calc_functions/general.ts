import { altitude_decorator, distance_decorator, duration_decorator, speed_decorator, sys_time_decorator, tel_time_decorator, time_decorator } from "../decorators";
import { priorityFunction, CalculatedValue } from "../function";
import { findIntersectionsSorted, iterateSynchronizedByTime, Range } from "../mandala_object";
import { generateTimeObject, getTelTime } from "../time";

export const landing_time = () => priorityFunction(time_decorator)
    .params({
        vx: "est.lpos.ax"
    }).call((data) => {
        let min_index = data.vx.data.reduce((min_ind, curr, curr_index, arr) => {
            if (arr[min_ind].value > curr.value) return curr_index;
            return min_ind;
        }, 0)

        const tel_time = data.vx.data[min_index].time_range.start

        return generateTimeObject(tel_time)
    });

export const takeoff_time = () => priorityFunction(time_decorator)
    .params({
        vx: "est.lpos.vx",
        vy: "est.lpos.vy",
        vz: "est.lpos.vz"
    }).call((data) => {
        let vx_continuous = data.vx.getContinuesRanges((value) => {
            return value.value > 5;
        })

        let vy_continuous = data.vx.getContinuesRanges((value) => {
            return value.value < 5;
        })

        let vz_continuous = data.vx.getContinuesRanges((value) => {
            return value.value > 0.5;
        })

        let time_interceptions = findIntersectionsSorted([vx_continuous, vy_continuous, vz_continuous]).filter((range) => {
            return range.end - range.start >= 1
        })

        if (time_interceptions.length === 0) return undefined

        return generateTimeObject(time_interceptions[0].start);
    })
    .params({
        speed: "est.pos.speed",
        vspeed: "est.pos.vspeed",
    }
    ).call(((data) => {
        let vspeed_continues_ranges = data.vspeed.getContinuesRanges((el) => {
            return el.value > 0.5;
        });
        let speed_continues_ranges = data.speed.getContinuesRanges((el) => {
            return el.value > 5;
        });

        let time_interceptions = findIntersectionsSorted([speed_continues_ranges, vspeed_continues_ranges]).filter((range) => {
            return range.end - range.start >= 1
        })

        if (time_interceptions.length === 0) return undefined

        return generateTimeObject(time_interceptions[0].start);
    }))

export const takeoff_speed = () => priorityFunction(speed_decorator)
    .params(
        {
            speed: "est.pos.speed"
        }).call((data) => {
            const takeoff_time_obj = getTelTime(takeoff_time())

            if (takeoff_time_obj.isError()) return undefined

            let speeds = data.speed.sliceByTime({
                start: takeoff_time_obj.value(),
                end: takeoff_time_obj.value(),
            })
            if (speeds.data.length == 0) return undefined

            return speeds.data[0].value
        });

export const landing_speed = () => priorityFunction(speed_decorator)
    .params({
        speed: "est.pos.speed"
    }).call((data) => {
        const landing_time_obj = getTelTime(landing_time())

        if (landing_time_obj.isError()) return undefined

        let speeds = data.speed.sliceByTime({
            start: landing_time_obj.value(),
            end: landing_time_obj.value(),
        })
        if (speeds.data.length == 0) return undefined

        return speeds.data[0].value
    });

export const max_altitude_by_mode = (mode: string) => priorityFunction(altitude_decorator)
    .params({
        alt: "est.pos.altitude",
        proc: "cmd.proc.mode"
    }).call((data) => {
        if (data.proc.meta.enum[mode] === undefined) return undefined

        const proc_index = Number(data.proc.meta.enum[mode])
        const command_ranges = data.proc.getContinuesRanges((el) => {
            return el.value === proc_index
        });
        const interceptions_maxes = findIntersectionsSorted([[data.alt], command_ranges]).map((inter) => {
            const curr_alt = data.alt.sliceByTime(inter)
            return Math.max(...curr_alt.data.map(el => el.value))
        })

        if (interceptions_maxes.length === 0) return undefined

        return Math.max(...interceptions_maxes)
    });


export const max_altitude = () => priorityFunction(altitude_decorator)
    .params({
        alt: "est.pos.altitude"
    }).call((data) => {
        return Math.max(...data.alt.data.map(el => el.value))
    });
