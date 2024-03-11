import { calculationFunction, preheatFunction } from "./function";

export const declareTextValue = (name: string, title: string) => preheatFunction()
    .call(() => {
        UserData.registerTextField(name, title === "" ? name : title);
    })

export const declareEnumValue = (name: string, title: string, enum_strings: Array<string>) => preheatFunction()
    .call(() => {
        UserData.registerEnum(name, title, enum_strings);
    })

export const getUserValue = (name: string) => calculationFunction()
    .call(() => {
        let temp = UserData.getFromField(name);
        return temp
    })
