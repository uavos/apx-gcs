import { decorators } from "./decorators";
import { CalculatedValue, calculationFunction, wrap } from "./function";

export interface Time {
    tel_time: CalculatedValue,
    sys_time: CalculatedValue
}

export function generateTimeObject(tel_time: number) {
    const wrapped_tel_time = wrap(tel_time, decorators.tel_time)
    return {
        tel_time: wrapped_tel_time,
        sys_time: systime(wrapped_tel_time)
    }
}

export function getTelTime(time: CalculatedValue): CalculatedValue {
    if (time.isError()) return time
    return time.value().tel_time
}

export const systime = (tel_time: CalculatedValue) => calculationFunction(decorators.sys_time)
    .params({
        sys_time: "est.sys.time"
    }).call((data) => {
        if (tel_time.isError()) return undefined
        const fonded_time = data.sys_time.sliceByTime({
            start: tel_time.value(),
            end: tel_time.value()
        })

        if (fonded_time.data.length === 0) return undefined

        return fonded_time.data[0].value
    });


