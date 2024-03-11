import { decorators } from "../decorators";
import { f_val_in_mandala_path_by_time_func } from "../fabrics"
import { calculationFunction } from "../function";
import { findIntersectionsSorted } from "../mandala_object";
import { generateTimeObject } from "../time";

export namespace touchdown {

    export const time = () => calculationFunction(decorators.time)
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

    export namespace speed {
        export const ias = f_val_in_mandala_path_by_time_func("est.air.airspeed", time, decorators.speed)
        export const ground_speed = f_val_in_mandala_path_by_time_func("est.pos.speed", time, decorators.speed)
    }
}

export namespace liftoff {

    export const time = () => calculationFunction(decorators.time)
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

    export namespace speed {
        export const ias = f_val_in_mandala_path_by_time_func("est.air.airspeed", time, decorators.speed)
        export const ground_speed = f_val_in_mandala_path_by_time_func("est.pos.speed", time, decorators.speed)
    }
}
