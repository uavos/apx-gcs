
export interface RawTelemetryValue {
    value: number,
    time: number
}

export interface IMandalaMeta {
    enum?: any,
    units?: string,
    uid?: number
}

export interface IMandala {
    raw_data?: Array<RawTelemetryValue>,
    meta?: IMandalaMeta;
    available: boolean
}

export function accessRaw(path: string): IMandala {
    let props: string[] = path.split(".");
    let currentObj: any = RawTelemetry;

    for (let prop of props) {
        if (currentObj && currentObj.hasOwnProperty(prop)) {
            currentObj = currentObj[prop];
        } else {
            throw new Error("Cannot find mandala path for " + path);
        }
    }

    return currentObj;
}
