"use strict";

(() =>
{
    let currentCharacter =
        null;


    function normalize(
        value)
    {
        if (!value ||
            typeof value !==
                "object")
        {
            return null;
        }

        const appearance =
            Array.isArray(
                value.appearance)
                ? value.appearance
                    .filter(
                        part =>
                            part &&
                            typeof part.group ===
                                "string" &&
                            Number.isFinite(
                                Number(
                                    part.itemType)))
                    .map(
                        part =>
                            ({
                                group:
                                    String(
                                        part.group),

                                itemType:
                                    Number(
                                        part.itemType),

                                colour:
                                    Number.isInteger(
                                        Number(
                                            part.colour)) &&
                                    Number(part.colour) >= 0 &&
                                    Number(part.colour) <= 0xFFFFFF
                                        ? Number(part.colour)
                                        : 0xFFFFFF
                            }))
                : [];

        return {
            id:
                String(
                    value.id ||
                    ""),

            name:
                String(
                    value.name ||
                    ""),

            appearance:
                appearance
        };
    }


    function set(
        value)
    {
        currentCharacter =
            normalize(
                value);

        return
            currentCharacter;
    }


    function clear()
    {
        currentCharacter =
            null;
    }


    function current()
    {
        return
            currentCharacter;
    }


    function appearanceFields(
        character =
            currentCharacter)
    {
        if (!character ||
            !Array.isArray(
                character.appearance))
        {
            return [];
        }

        const fields =
            [];

        for (const part of
             character.appearance)
        {
            if (!part ||
                !part.group)
            {
                continue;
            }

            const itemType =
                Number(
                    part.itemType);

            if (!Number.isFinite(
                    itemType) ||
                itemType <= 0)
            {
                continue;
            }

            fields.push(
                part.group);

            fields.push(
                itemType);

            const colour =
                Number(
                    part.colour);

            fields.push(
                Number.isInteger(colour) &&
                colour >= 0 &&
                colour <= 0xFFFFFF
                    ? colour
                    : 0xFFFFFF);
        }

        return fields;
    }


    window.CharacterStore =
    {
        set:
            set,

        clear:
            clear,

        current:
            current,

        appearanceFields:
            appearanceFields
    };
})();
