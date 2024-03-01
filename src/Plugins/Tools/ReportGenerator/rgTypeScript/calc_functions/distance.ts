import { distance_decorator } from "../decorators";
import { priorityFunction } from "../function";
import { landing_time, takeoff_time } from "./general";
import { getTelTime } from "../time";
import { findIntersectionsSorted, iterateSynchronizedByTime, Range } from "../mandala_object";

export const overall_distance_in_flight = () => priorityFunction(distance_decorator)
    .params({
        speed: "est.pos.speed",
    }).call((data) => {
        const takeoff_time_obj = getTelTime(takeoff_time())
        const landing_time_obj = getTelTime(landing_time())

        if (takeoff_time_obj.isError() || landing_time_obj.isError()) return undefined
        if (takeoff_time_obj.value() > landing_time_obj.value()) return undefined

        const flight_range: Range = {
            start: takeoff_time_obj.value(),
            end: landing_time_obj.value()
        }

        let overall_distance = 0

        data.speed.sliceByTime(flight_range).data.forEach(el => {
            let diff = el.time_range.end - el.time_range.start
            overall_distance += diff * el.value
        })

        return overall_distance / 1000
    });
