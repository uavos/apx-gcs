import { decorators } from "../decorators";
import { f_val_in_mandala_path_by_time_func } from "../fabrics";
import { calculationFunction, CalculatedValue, DecoratorFunction } from "../function";
import { findIntersectionsSorted, iterateSynchronizedByTime, Mandala, Range } from "../mandala_object";
import { generateTimeObject, getTelTime } from "../time";

export const max_altitude_by_mode = (mode: string) => calculationFunction(decorators.altitude)
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

export const min_altitude_by_mode = (mode: string) => calculationFunction(decorators.altitude)
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
            return Math.min(...curr_alt.data.map(el => el.value))
        })

        if (interceptions_maxes.length === 0) return undefined

        return Math.min(...interceptions_maxes)
    });

export const max_climb_rate_by_mode = (mode: string) => calculationFunction(decorators.speed)
    .params({
        vspeed: "est.pos.vspeed",
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
            let prev_alt = +Infinity;
            const only_on_climb = curr_alt.getContinuesRanges(el => {
                const is_climbing = el.value > prev_alt;
                prev_alt = el.value;
                return is_climbing
            })

            const vspeed_on_climbs_maxes = findIntersectionsSorted([only_on_climb, [data.vspeed]]).map(el => {
                const curr_vspeeds = data.vspeed.sliceByTime(el)
                return Math.max(...curr_vspeeds.data.map(el => el.value))
            })

            return Math.max(...vspeed_on_climbs_maxes)
        })

        if (interceptions_maxes.length === 0) return undefined

        return Math.min(...interceptions_maxes)
    });

export const max_value_by_mandala = (path: string, decorator: DecoratorFunction | null) => calculationFunction(decorator)
    .params({
        mandala: path
    }).call((data) => {
        return Math.max(...data.mandala.data.map(el => el.value))
    })

export const min_value_by_mandala = (path: string, decorator: DecoratorFunction | null) => calculationFunction(decorator)
    .params({
        mandala: path
    }).call((data) => {
        return Math.min(...data.mandala.data.map(el => el.value))
    })

export const mode_mandala_object = (mode: string) => calculationFunction()
    .params(
        {
            proc: "cmd.proc.mode"
        }
    ).call(data => {
        if (data.proc.meta.enum[mode] === undefined) return undefined

        const proc_index = Number(data.proc.meta.enum[mode])
        const command_ranges = data.proc.getContinuesRanges((el) => {
            return el.value === proc_index
        });

        if (command_ranges.length == 0) return undefined

        return command_ranges
    })

export const mode_last_mandala_object = (mode: string) => calculationFunction()
    .params(
        {
            proc: "cmd.proc.mode"
        }
    ).call(data => {
        if (data.proc.meta.enum[mode] === undefined) return undefined

        const proc_index = Number(data.proc.meta.enum[mode])
        const command_ranges = data.proc.getContinuesRanges((el) => {
            return el.value === proc_index
        });

        if (command_ranges.length == 0) return undefined

        return command_ranges[command_ranges.length - 1]
    })

export const mode_first_mandala_object = (mode: string) => calculationFunction()
    .params(
        {
            proc: "cmd.proc.mode"
        }
    ).call(data => {
        if (data.proc.meta.enum[mode] === undefined) return undefined

        const proc_index = Number(data.proc.meta.enum[mode])
        const command_ranges = data.proc.getContinuesRanges((el) => {
            return el.value === proc_index
        });

        if (command_ranges.length == 0) return undefined

        return command_ranges[0]
    })

export const begin_time = () => calculationFunction(decorators.speed)
    .params({
        mode: "cmd.proc.mode",
        stage: "cmd.proc.stage"
    })
    .call(data => {
        const mode_name = "TAKEOFF"
        const stage_num = 2;

        const stage_first = mode_first_mandala_object(mode_name)

        if (stage_first.isError()) return undefined

        const stage_ranges = data.stage.sliceByTime(stage_first.value()).getContinuesRanges((el) => el.value === stage_num)
        if (stage_ranges.length === 0) return undefined

        return generateTimeObject(stage_ranges[0].start)
    })



