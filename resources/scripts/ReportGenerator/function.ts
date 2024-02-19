import { IMandala, accessRaw } from "./bridge";
import { Mandala } from "./mandala_object";

type FunctionParams = { [key: string]: string }
type CallBackParams = { [key: string]: Mandala }

class CalculatedValue {
    public is_calculated = true
    private _is_error: boolean;
    private _what: string;
    private _value: any;
    private _callParams: CallBackParams;
    private _allParamsAvailable: boolean;

    constructor() {
        this._is_error = true;
        this._what = "Did not find any mandala set that fits function requirements";
        this._callParams = {};
        this._allParamsAvailable = false;
        this._value = undefined
    }

    public params(objParams: FunctionParams): this {
        if (this.isError() === false) return this

        this._callParams = Object.assign({}, ...Object.entries(objParams).map(([key, value]) => {
            return {
                [key]: new Mandala({ mandala_object: accessRaw(value) })
            };
        }));
        this._allParamsAvailable = !Object.values(this._callParams).some(value => {
            return !value.isAvailable();
        });
        return this;
    }

    public call(func: (params: CallBackParams) => any): this {
        if (this.isError() === false || !this._allParamsAvailable) return this;

        const temp = func(this._callParams);

        if (temp === undefined) {
            return this;
        }

        this._is_error = false;
        this._value = temp;

        return this
    }

    public isError() {
        return this._is_error
    }

    public value() {
        return this._value
    }

    public what() {
        return this._what
    }
}

export function priorityFunction() {
    return new CalculatedValue()
}
