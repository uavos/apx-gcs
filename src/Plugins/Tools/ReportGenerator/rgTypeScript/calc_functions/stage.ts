import { decorators } from "../decorators";
import { f_min_in_mandala_path_by_range, f_max_in_mandala_path_by_range, f_speed_by_range_func, f_alt_by_range_func } from "../fabrics";
import { CalculatedValue, DecoratorFunction, calculationFunction } from "../function";
import { Mandala, Range, iterateSynchronizedByTime } from "../mandala_object";
import { generateTimeObject, getTelTime } from "../time";
import { mode_first_mandala_object, mode_last_mandala_object } from "./general";
import { liftoff, touchdown } from "./moment";

export const max_IAS_by_range = f_max_in_mandala_path_by_range("est.air.airspeed", decorators.speed)
export const min_IAS_by_range = f_min_in_mandala_path_by_range("est.air.airspeed", decorators.speed)

export const max_TAS_by_range = (range: CalculatedValue) => calculationFunction(decorators.speed)
    .params({
        airspeed: "est.air.airspeed",
        ktas: "est.air.ktas"
    }).call(data => {
        if (range.isError()) return undefined;

        let rising_stage_range_v = range.value() as Range;

        const airspeed_in_range = data.airspeed.sliceByTime(rising_stage_range_v).data
        const ktas_in_range = data.ktas.sliceByTime(rising_stage_range_v).data

        if (airspeed_in_range.length === 0 || ktas_in_range.length === 0) return undefined

        const TAS_speeds: Array<number> = []
        iterateSynchronizedByTime(airspeed_in_range, ktas_in_range, (air_el, ktas_el) => {
            TAS_speeds.push(air_el.value * ktas_el.value)
        })

        return Math.max(...TAS_speeds)
    })

export const min_TAS_by_range = (range: CalculatedValue) => calculationFunction(decorators.speed)
    .params({
        airspeed: "est.air.airspeed",
        ktas: "est.air.ktas"
    }).call(data => {
        if (range.isError()) return undefined;

        let rising_stage_range_v = range.value() as Range;

        const airspeed_in_range = data.airspeed.sliceByTime(rising_stage_range_v).data
        const ktas_in_range = data.ktas.sliceByTime(rising_stage_range_v).data

        if (airspeed_in_range.length === 0 || ktas_in_range.length === 0) return undefined

        const TAS_speeds: Array<number> = []
        iterateSynchronizedByTime(airspeed_in_range, ktas_in_range, (air_el, ktas_el) => {
            TAS_speeds.push(air_el.value * ktas_el.value)
        })

        return Math.min(...TAS_speeds)
    })

export const max_HMSL_by_range = f_max_in_mandala_path_by_range("est.pos.hmsl", decorators.altitude)
export const min_HMSL_by_range = f_min_in_mandala_path_by_range("est.pos.hmsl", decorators.altitude)
export const max_AGL_by_range = f_max_in_mandala_path_by_range("est.pos.agl", decorators.altitude)
export const min_AGL_by_range = f_min_in_mandala_path_by_range("est.pos.agl", decorators.altitude)

export const max_vspeed_by_range = f_max_in_mandala_path_by_range("est.pos.vspeed", decorators.speed)
export const min_vspeed_by_range = f_min_in_mandala_path_by_range("est.pos.vspeed", decorators.speed)


export namespace running {
    export const start = () => calculationFunction(decorators.time)
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

    export const end = liftoff.time

    export const time_range = () => calculationFunction()
        .params({}).call(data => {
            const running_start_fact = getTelTime(start())
            const liftoff_time = getTelTime(end())

            if (running_start_fact.isError()) return undefined;
            if (liftoff_time.isError()) return undefined;

            return {
                start: running_start_fact.value(),
                end: liftoff_time.value()
            } as Range
        })

    export namespace tas {
        export const min = f_speed_by_range_func(min_TAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_TAS_by_range, time_range)
    }

    export namespace ias {
        export const min = f_speed_by_range_func(min_IAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_IAS_by_range, time_range)
    }

    export namespace agl {
        export const min = f_alt_by_range_func(min_AGL_by_range, time_range)
        export const max = f_alt_by_range_func(max_AGL_by_range, time_range)
    }

    export namespace hmsl {
        export const min = f_alt_by_range_func(min_HMSL_by_range, time_range)
        export const max = f_alt_by_range_func(max_HMSL_by_range, time_range)
    }

    export namespace vspeed {
        export const min = f_speed_by_range_func(min_vspeed_by_range, time_range)
        export const max = f_speed_by_range_func(max_vspeed_by_range, time_range)
    }
}

export namespace takeoff {
    export const time_range = () => calculationFunction()
        .params({}).call(data => {
            const mode_name = "TAKEOFF"
            const takeoff_fact = getTelTime(liftoff.time())
            const takeoff_mode = mode_first_mandala_object(mode_name)

            if (takeoff_fact.isError()) return undefined;
            if (takeoff_mode.isError()) return undefined;

            const takeoff_mode_v = takeoff_mode.value() as Mandala

            return {
                start: takeoff_fact.value(),
                end: takeoff_mode_v.end
            } as Range
        })

    export namespace tas {
        export const min = f_speed_by_range_func(min_TAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_TAS_by_range, time_range)
    }

    export namespace ias {
        export const min = f_speed_by_range_func(min_IAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_IAS_by_range, time_range)
    }

    export namespace agl {
        export const min = f_alt_by_range_func(min_AGL_by_range, time_range)
        export const max = f_alt_by_range_func(max_AGL_by_range, time_range)
    }

    export namespace hmsl {
        export const min = f_alt_by_range_func(min_HMSL_by_range, time_range)
        export const max = f_alt_by_range_func(max_HMSL_by_range, time_range)
    }

    export namespace vspeed {
        export const min = f_speed_by_range_func(min_vspeed_by_range, time_range)
        export const max = f_speed_by_range_func(max_vspeed_by_range, time_range)
    }


}

export namespace flight {
    export const time_range = () => calculationFunction()
        .params({}).call(data => {

            let rising_stage_range = takeoff.time_range()
            let landing_stage_range = landing.time_range()

            if (rising_stage_range.isError() || landing_stage_range.isError()) return undefined;

            let rising_stage_range_v = rising_stage_range.value() as Range;
            let landing_stage_range_v = landing_stage_range.value() as Range;

            return {
                start: rising_stage_range_v.end,
                end: landing_stage_range_v.start
            } as Range
        })

    export namespace tas {
        export const min = f_speed_by_range_func(min_TAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_TAS_by_range, time_range)
    }

    export namespace ias {
        export const min = f_speed_by_range_func(min_IAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_IAS_by_range, time_range)
    }

    export namespace agl {
        export const min = f_alt_by_range_func(min_AGL_by_range, time_range)
        export const max = f_alt_by_range_func(max_AGL_by_range, time_range)
    }

    export namespace hmsl {
        export const min = f_alt_by_range_func(min_HMSL_by_range, time_range)
        export const max = f_alt_by_range_func(max_HMSL_by_range, time_range)
    }

    export namespace vspeed {
        export const min = f_speed_by_range_func(min_vspeed_by_range, time_range)
        export const max = f_speed_by_range_func(max_vspeed_by_range, time_range)
    }
}

export namespace landing {
    export const time_range = () => calculationFunction()
        .params({}).call(data => {
            const mode_name = "LANDING"
            const landing_fact = getTelTime(touchdown.time())
            const landing_mode = mode_last_mandala_object(mode_name)

            if (landing_fact.isError()) return undefined;
            if (landing_mode.isError()) return undefined;

            const landing_mode_v = landing_mode.value() as Mandala

            return {
                start: landing_mode_v.start,
                end: landing_fact.value()
            } as Range
        })

    export namespace tas {
        export const min = f_speed_by_range_func(min_TAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_TAS_by_range, time_range)
    }

    export namespace ias {
        export const min = f_speed_by_range_func(min_IAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_IAS_by_range, time_range)
    }

    export namespace agl {
        export const min = f_alt_by_range_func(min_AGL_by_range, time_range)
        export const max = f_alt_by_range_func(max_AGL_by_range, time_range)
    }

    export namespace hmsl {
        export const min = f_alt_by_range_func(min_HMSL_by_range, time_range)
        export const max = f_alt_by_range_func(max_HMSL_by_range, time_range)
    }

    export namespace vspeed {
        export const min = f_speed_by_range_func(min_vspeed_by_range, time_range)
        export const max = f_speed_by_range_func(max_vspeed_by_range, time_range)
    }

    export const landing_overload = () => calculationFunction()
        .params({
            az: "est.lpos.az"
        }).call(data => {
            let range = time_range();
            if (range.isError()) return undefined;
            let range_v = range.value() as Range
            const sliced = data.az.sliceByTime({
                start: range_v.end,
                end: range_v.end
            })

            if (sliced.data.length === 0) return undefined

            return sliced.data[0].value / -9.8
        })
}

export namespace rollout {
    export const start = touchdown.time
    export const end = () => calculationFunction(decorators.time)
        .params({
            speed: "est.pos.speed"
        })
        .call(data => {
            const speed_less_then_one = data.speed.getContinuesRanges(el => el.value < 1)

            if (speed_less_then_one.length === 0) return undefined

            return generateTimeObject(speed_less_then_one[speed_less_then_one.length - 1].end)
        })

    export const time_range = () => calculationFunction()
        .params({}).call(data => {
            const touchdown_time = getTelTime(start())
            const stopping_time = getTelTime(end())

            if (touchdown_time.isError()) return undefined;
            if (stopping_time.isError()) return undefined;

            return {
                start: touchdown_time.value(),
                end: stopping_time.value()
            } as Range
        })

    export namespace tas {
        export const min = f_speed_by_range_func(min_TAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_TAS_by_range, time_range)
    }

    export namespace ias {
        export const min = f_speed_by_range_func(min_IAS_by_range, time_range)
        export const max = f_speed_by_range_func(max_IAS_by_range, time_range)
    }

    export namespace agl {
        export const min = f_alt_by_range_func(min_AGL_by_range, time_range)
        export const max = f_alt_by_range_func(max_AGL_by_range, time_range)
    }

    export namespace hmsl {
        export const min = f_alt_by_range_func(min_HMSL_by_range, time_range)
        export const max = f_alt_by_range_func(max_HMSL_by_range, time_range)
    }

    export namespace vspeed {
        export const min = f_speed_by_range_func(min_vspeed_by_range, time_range)
        export const max = f_speed_by_range_func(max_vspeed_by_range, time_range)
    }
}
