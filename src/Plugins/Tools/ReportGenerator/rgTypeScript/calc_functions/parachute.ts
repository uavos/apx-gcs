import { Mandala, findIntersectionsSorted } from "../mandala_object";
import { CalculatedValue, DecoratorFunction, calculationFunction } from "../function";
import { generateTimeObject, getTelTime as get_tel_time } from "../time";
import { f_val_in_mandala_path_by_time_func } from "../fabrics";
import { touchdown } from "moment"
import { decorators } from "../decorators";

export const launch_time = () => calculationFunction(decorators.time)
    .params({
        launch: "ctr.ers.launch"
    }
    ).call((data) => {
        const enum_ON_value = data.launch.meta.enum["on"]

        const temp = data.launch.data.find((val) => {
            return val.value == enum_ON_value
        })

        if (temp === undefined) return undefined

        return generateTimeObject(temp.time_range.start)
    });

export namespace open {
    export namespace alt {
        export const hmsl = f_val_in_mandala_path_by_time_func("est.pos.hmsl", launch_time, decorators.altitude)
        export const agl = f_val_in_mandala_path_by_time_func("est.pos.agl", launch_time, decorators.altitude)
    }

    export namespace speed {
        export const ias = f_val_in_mandala_path_by_time_func("est.air.airspeed", launch_time, decorators.speed)
    }
}

export namespace avg {
    export const landing_speed = () => calculationFunction(decorators.speed)
        .params({
            vspeed: "est.pos.vspeed"
        }).call((data) => {
            const launch_time_obj = get_tel_time(launch_time())
            const landing_time_obj = get_tel_time(touchdown.time())

            if (launch_time_obj.isError()) return undefined
            if (landing_time_obj.isError()) return undefined
            const launch_time_ = launch_time_obj.value()
            const landing_time_ = landing_time_obj.value()

            const speed_values_on_landing = data.vspeed.sliceByTime({
                start: launch_time_,
                end: landing_time_
            })

            const average_vspeed = speed_values_on_landing.data.reduce((a, b) => a + b.value, 0) / speed_values_on_landing.data.length;

            return average_vspeed
        });
}



