import { CalculatedValue, DecoratorFunction, calculationFunction } from "./function"

import { Mandala, Range, iterateSynchronizedByTime } from "mandala_object";
import { getTelTime } from "./time";
import { decorators } from "./decorators";

export function f_by_range_func(func: (_: CalculatedValue) => CalculatedValue, get_range: () => CalculatedValue, decorator: DecoratorFunction | null) {
    return () => calculationFunction(decorator)
        .params({}).call(data => {
            const min_hmsl = func(get_range())
            if (min_hmsl.isError()) return undefined

            return min_hmsl.value()
        })
}

export function f_alt_by_range_func(func: (_: CalculatedValue) => CalculatedValue, get_range: () => CalculatedValue) {
    return f_by_range_func(func, get_range, decorators.altitude)
}

export function f_speed_by_range_func(func: (_: CalculatedValue) => CalculatedValue, get_range: () => CalculatedValue) {
    return f_by_range_func(func, get_range, decorators.speed)
}

export function f_min_in_mandala_path_by_range(mandala_path: string, decorator: DecoratorFunction | null) {
    return (range: CalculatedValue) => calculationFunction(decorator)
        .params({
            obj: mandala_path
        }).call(data => {
            if (range.isError()) return undefined;
            const range_v = range.value() as Range

            const values_in_range = data.obj.sliceByTime(range_v).data.map(el => el.value)

            if (values_in_range.length === 0) return undefined

            return Math.min(...values_in_range)
        })
}

export function f_max_in_mandala_path_by_range(mandala_path: string, decorator: DecoratorFunction | null) {
    return (range: CalculatedValue) => calculationFunction(decorator)
        .params({
            obj: mandala_path
        }).call(data => {
            if (range.isError()) return undefined;
            const range_v = range.value() as Range

            const values_in_range = data.obj.sliceByTime(range_v).data.map(el => el.value)

            if (values_in_range.length === 0) return undefined

            return Math.max(...values_in_range)
        })
}

export function f_val_in_mandala_path_by_time_func(mandala_path: string, time_func: () => CalculatedValue, decorator: DecoratorFunction) {
    return () => calculationFunction(decorator)
        .params({
            obj: mandala_path
        }).call((data) => {
            const time_obj = getTelTime(time_func())

            if (time_obj.isError()) return undefined
            const time_v = time_obj.value()

            const value_on_time = data.obj.sliceByTime({
                start: time_v,
                end: time_v
            }).data

            if (value_on_time.length === 0) return undefined

            return value_on_time[0].value
        });
}
