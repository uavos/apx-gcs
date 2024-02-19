import { Mandala, findIntersectionsSorted } from "./mandala_object";
import { priorityFunction } from "./function";
import { landing_time } from "./general";

export const launch_time = () => priorityFunction()
    .params({
        launch: "ctr.ers.launch"
    }
    ).call((data) => {
        const enum_ON_value = data.launch.meta.enum["on"]

        const temp = data.launch.data.find((val) => {
            return val.value == enum_ON_value
        })

        return temp === undefined ? undefined : temp.time_range.start
    });

export const open_altitude = () => priorityFunction()
    .params({
        alt: "est.pos.altitude"
    }).call((data) => {
        const launch_time_obj = launch_time()

        if (launch_time_obj.isError()) return undefined
        const launch_time_ = launch_time_obj.value()

        const altitude_value_on_time = data.alt.data.find(el => {
            return el.time_range.start >= launch_time_ && launch_time_ <= el.time_range.end
        })

        return altitude_value_on_time === undefined ? undefined : altitude_value_on_time.value
    });

export const open_speed = () => priorityFunction()
    .params({
        speed: "est.pos.speed",
    }
    ).call((data) => {
        const launch_time_obj = launch_time()

        if (launch_time_obj.isError()) return undefined
        const launch_time_ = launch_time_obj.value()

        const speed_value_on_time = data.speed.data.find(el => {
            return el.time_range.start >= launch_time_ && launch_time_ <= el.time_range.end
        })

        return speed_value_on_time === undefined ? undefined : speed_value_on_time.value
    });

export const average_landing_speed = () => priorityFunction()
    .params({
        speed: "est.pos.speed"
    }).call((data) => {
        const launch_time_obj = launch_time()
        const landing_time_obj = landing_time()

        if (launch_time_obj.isError()) return undefined
        if (landing_time_obj.isError()) return undefined
        const launch_time_ = launch_time_obj.value()
        const landing_time_ = landing_time_obj.value()

        const speed_values_on_landing = data.speed.sliceByTime({
            start: launch_time_,
            end: landing_time_
        })

        const average_speed = speed_values_on_landing.data.reduce((a, b) => a + b.value, 0) / speed_values_on_landing.data.length;

        return average_speed
    });