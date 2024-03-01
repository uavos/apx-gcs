import { IMandala, IMandalaMeta, RawTelemetryValue } from "./bridge"

export interface CallbackOneParam<T1, T2 = void> {
    (param1: T1): T2;
}

export interface Range {
    start: number,
    end: number
}

class DataElement {
    value: number
    time_range: Range
    index_range: Range
}

export function iterateSynchronizedByTime(arr1: DataElement[], arr2: DataElement[], callback: (element1: DataElement, element2: DataElement) => void) {
    let i = 0;
    let j = 0;

    while (i < arr1.length && j < arr2.length) {
        const element1 = arr1[i];
        const element2 = arr2[j];

        if (element1.time_range.start <= element2.time_range.end && element1.time_range.end >= element2.time_range.start) {
            callback(element1, element2);

            if (element1.time_range.end <= element2.time_range.end) {
                i++;
            } else {
                j++;
            }
        } else if (element1.time_range.end < element2.time_range.start) {
            i++;
        } else {
            j++;
        }
    }
}

export function findIntersectionsSorted(ranges: Array<Array<Range>>): Array<Range> {
    if (ranges.length === 0) {
        return [];
    }

    let intersections: Array<Range> = ranges[0];

    for (let i = 1; i < ranges.length; i++) {
        const currentRanges = ranges[i];
        let newIntersections: Array<Range> = [];

        let j = 0;
        let k = 0;

        while (j < intersections.length && k < currentRanges.length) {
            const intersection = getIntersection(intersections[j], currentRanges[k]);

            if (intersection) {
                newIntersections.push(intersection);
            }

            if (intersections[j].end < currentRanges[k].end) {
                j++;
            } else {
                k++;
            }
        }

        intersections = newIntersections;
    }

    return intersections;
}

function getIntersection(lhs: Range, rhs: Range): Range | null {
    const start = Math.max(lhs.start, rhs.start);
    const end = Math.min(lhs.end, rhs.end);

    if (start <= end) {
        return { start, end };
    }

    return null;
}

export class Mandala implements Range {
    private _data_array: Array<DataElement>
    private _meta: IMandalaMeta
    private _available: boolean

    public constructor(params: { mandala_object?: IMandala, tel_data_array?: Array<DataElement> }) {
        if (params.mandala_object) {
            this._available = params.mandala_object.available
            if (!this._available) return
            this._data_array = this.mapArray(params.mandala_object.raw_data)
            this._meta = { ...params.mandala_object.meta }
            return
        }
        if (params.tel_data_array) {
            this._data_array = params.tel_data_array
            this._available = true
            return
        }

        this._data_array = []
    }

    public getElementByTime(time: number): DataElement | null {
        this._data_array.forEach(el => {
            if (el.time_range.start >= time && time < el.time_range.end) return el;
        });

        return null;
    }

    public getContinuesRanges(pred: CallbackOneParam<DataElement, boolean>): Array<Mandala> {
        const output: Array<Mandala> = [];
        let is_in_range = false;
        let range_start = 0;

        this._data_array.forEach((element, index, arr) => {
            let is_right_value = pred(element)

            if (is_right_value) {
                if (!is_in_range) {
                    is_in_range = true;
                    range_start = index;
                    return;
                }
            } else if (is_in_range) {
                output.push(new Mandala({ tel_data_array: arr.slice(range_start, index) }))
                is_in_range = false;
            }
        });
        if (is_in_range) {
            output.push(new Mandala({ tel_data_array: this._data_array.slice(range_start) }))
        }

        return output
    }

    public sliceByTime(range: Range): Mandala {
        const temp = this._data_array.filter(element => {
            return element.time_range.start <= range.end && element.time_range.end >= range.start;
        });

        return new Mandala({ tel_data_array: temp })
    }

    public get meta() {
        return this._meta
    }

    public get data(): Array<DataElement> {
        return this._data_array
    }

    public get duration(): number {
        return this.end - this.start
    }

    public get start(): number {
        if (this._data_array.length == 0) return 0;
        return this._data_array[0].time_range.start;
    }

    public get end(): number {
        if (this._data_array.length == 0) return 0;
        return this._data_array[this._data_array.length - 1].time_range.end;
    }

    public isAvailable() {
        return this._available
    }

    public findIntersectionsByTimeWith(other: Mandala): Array<Range> {
        return findIntersectionsSorted([this._data_array.map(el => el.time_range), other._data_array.map(el => el.time_range)]);
    }

    private mapArray(raw_array: Array<RawTelemetryValue>): Array<DataElement> {
        if (raw_array.length === 0) return [];

        const output: Array<DataElement> = [];
        let last_value = raw_array[0].value;
        let last_time = raw_array[0].time;
        let last_index = 0;

        raw_array.forEach((element, index) => {
            if (element.value != last_value) {
                output.push({
                    value: last_value,
                    time_range: {
                        start: last_time,
                        end: element.time
                    },
                    index_range: {
                        start: last_index,
                        end: index
                    }
                })
                last_value = element.value;
                last_time = element.time;
                last_index = index;
            }
        });

        if (raw_array.length - 1 != last_index) {
            output.push({
                value: last_value,
                time_range: {
                    start: last_time,
                    end: raw_array[raw_array.length - 1].time
                },
                index_range: {
                    start: last_index,
                    end: raw_array.length - 1
                }
            })
        }

        return output
    }
}
