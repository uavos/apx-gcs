import { calculationFunction } from "../function";
import { getTelTime } from "../time";
import { decorators } from "../decorators";
import { liftoff, touchdown } from "./moment";

export const overall_seconds_in_flight = () => calculationFunction(decorators.duration)
    .params({}).call((data) => {
        const takeoff_time_obj = getTelTime(liftoff.time())
        const landing_time_obj = getTelTime(touchdown.time())

        if (takeoff_time_obj.isError() || landing_time_obj.isError()) return undefined
        if (takeoff_time_obj.value() > landing_time_obj.value()) return undefined

        return landing_time_obj.value() - takeoff_time_obj.value()
    });
