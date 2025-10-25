import { calculationFunction } from "../function";
import { getTelTime } from "../time";
import { findIntersectionsSorted, iterateSynchronizedByTime, Range } from "../mandala_object";
import { decorators } from "../decorators";
import { liftoff, touchdown } from "./moment";

export const overall_distance_in_flight = () => calculationFunction(decorators.distance)
    .params({
        speed: "est.pos.speed",
    }).call((data) => {
        const takeoff_time_obj = getTelTime(liftoff.time())
        const landing_time_obj = getTelTime(touchdown.time())

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

function calculateDistance(lat1, lon1, lat2, lon2) {
    const R = 6371e3; // Radius of the Earth in meters
    const φ1 = lat1 * Math.PI / 180; // Convert latitude to radians
    const φ2 = lat2 * Math.PI / 180;
    const Δφ = (lat2 - lat1) * Math.PI / 180;
    const Δλ = (lon2 - lon1) * Math.PI / 180;

    const a = Math.sin(Δφ / 2) * Math.sin(Δφ / 2) +
        Math.cos(φ1) * Math.cos(φ2) *
        Math.sin(Δλ / 2) * Math.sin(Δλ / 2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

    const distance = R * c; // Distance in meters
    return distance;
}
