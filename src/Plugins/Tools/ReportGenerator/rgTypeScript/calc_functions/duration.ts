import { duration_decorator } from "../decorators";
import { priorityFunction } from "../function";
import { landing_time, takeoff_time } from "./general";
import { getTelTime } from "../time";

export const overall_seconds_in_flight = () => priorityFunction(duration_decorator)
    .params({}).call((data) => {
        const takeoff_time_obj = getTelTime(takeoff_time())
        const landing_time_obj = getTelTime(landing_time())

        if (takeoff_time_obj.isError() || landing_time_obj.isError()) return undefined
        if (takeoff_time_obj.value() > landing_time_obj.value()) return undefined

        return landing_time_obj.value() - takeoff_time_obj.value()
    });
