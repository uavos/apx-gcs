import { CalculatedValue } from "./function";
import { Time } from "./time";

function epochToDateTime(epoch) {
    let date = new Date(epoch * 1000);

    let datetime = date.toLocaleString('en-US', { timeZone: 'UTC' });

    return datetime;
}

function secondsToHMS(seconds) {
    let hours = Math.floor(seconds / 3600);
    let minutes = Math.floor((seconds % 3600) / 60);
    let remainingSeconds = Math.floor(seconds % 60);
    let milliseconds = Math.floor((seconds % 1) * 1000);

    let timeString = ('0' + hours).slice(-2) + ':' +
        ('0' + minutes).slice(-2) + ':' +
        ('0' + remainingSeconds).slice(-2) + '.' +
        ('00' + milliseconds).slice(-3);

    return timeString;
}

export namespace decorators {
    export function speed(value: any) {
        let true_type_value = Number(value)

        return true_type_value.toFixed(2) + " m/s"
    }

    export function distance(value: any) {
        let true_type_value = Number(value)
        if (true_type_value < 1) return (true_type_value * 1000).toFixed(2) + " m"
        return true_type_value.toFixed(2) + " km"
    }

    export function altitude(value: any) {
        let true_type_value = Number(value)

        return true_type_value.toFixed(2) + " m"
    }

    export function sys_time(value: any) {
        let true_type_value = Number(value)
        return epochToDateTime(true_type_value)
    }

    export function tel_time(value: any) {
        let true_type_value = Number(value)

        return true_type_value.toFixed(2) + " s"
    }

    export function duration(value: any) {
        let true_type_value = Number(value)

        return secondsToHMS(true_type_value)
    }

    export function voltage(value: any) {
        let true_type_value = Number(value)

        return true_type_value.toFixed(2) + " v"
    }

    export function acceleration(value: any) {
        let true_type_value = Number(value)

        return true_type_value.toFixed(2) + " m/s^2"
    }

    export function time(value: any) {
        let true_type_value = value as Time

        let sys_time = true_type_value.sys_time
        let tel_time = true_type_value.tel_time

        return "System time: [" + sys_time.stringify() + "] Telemetry time: [" + tel_time.stringify() + "]"
    }
}


