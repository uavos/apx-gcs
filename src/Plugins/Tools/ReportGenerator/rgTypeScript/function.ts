import { IMandala, accessRaw } from "./bridge";
import { Mandala } from "./mandala_object";

type FunctionParams = { [key: string]: string }
type CallBackParams = { [key: string]: Mandala }

type DecoratorFunction = (val: any) => string

export function wrap(val: any, decorator: DecoratorFunction | null = null) {
    return new CalculatedValue(decorator, val)
}

export class CalculatedValue {
    public is_calculated = true
    private _is_error: boolean;
    private _what: string;
    private _value: any;
    private _callParams: CallBackParams;
    private _allParamsAvailable: boolean;
    private _decorator_function: DecoratorFunction | null

    constructor(decorator_function: DecoratorFunction | null = null, default_value: any = undefined) {
        this._is_error = default_value === undefined;
        this._what = "Not detected";
        this._callParams = {};
        this._allParamsAvailable = false;
        this._value = default_value
        this._decorator_function = decorator_function
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

    public stringify() {
        if (this.isError()) return this.what()
        if (this._decorator_function === null) return this.value()

        return this._decorator_function(this.value())
    }
}

export function priorityFunction(func: DecoratorFunction | null = null) {
    return new CalculatedValue(func)
}
