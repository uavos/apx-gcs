import { decorators } from "../decorators";
import { calculationFunction } from "../function";
import { getTelTime } from "../time";
import { min_value_by_mandala } from "./general";
import { liftoff, touchdown } from "./moment";

export const takeoff_battery_voltage = () => calculationFunction(decorators.voltage)
    .params(
        {
            voltage: "sns.bat.voltage"
        }).call((data) => {
            const takeoff_time_obj = getTelTime(liftoff.time())

            if (takeoff_time_obj.isError()) return undefined

            let voltages = data.voltage.sliceByTime({
                start: takeoff_time_obj.value(),
                end: takeoff_time_obj.value(),
            })
            if (voltages.data.length == 0) return undefined

            return voltages.data[0].value
        });

export const landing_battery_voltage = () => calculationFunction(decorators.voltage)
    .params(
        {
            voltage: "sns.bat.voltage"
        }).call((data) => {
            const landing_time_obj = getTelTime(touchdown.time())

            if (landing_time_obj.isError()) return undefined

            let voltages = data.voltage.sliceByTime({
                start: landing_time_obj.value(),
                end: landing_time_obj.value(),
            })
            if (voltages.data.length == 0) return undefined

            return voltages.data[0].value
        });

export const lowest_battery_voltage = () => min_value_by_mandala("sns.bat.voltage", decorators.voltage)
