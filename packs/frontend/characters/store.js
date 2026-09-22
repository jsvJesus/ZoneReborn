"use strict";

(() =>
{
    const CHARACTERS_KEY =
        "zone.test.characters";

    const CURRENT_CHARACTER_KEY =
        "zone.test.currentCharacter";


    function load()
    {
        try
        {
            const raw =
                localStorage.getItem(
                    CHARACTERS_KEY);

            if (!raw)
            {
                return [];
            }

            const value =
                JSON.parse(
                    raw);

            return Array.isArray(value)
                ? value
                : [];
        }
        catch
        {
            return [];
        }
    }


    function save(
        characters)
    {
        localStorage.setItem(
            CHARACTERS_KEY,
            JSON.stringify(
                characters));
    }


    function currentIndex()
    {
        const characters =
            load();

        if (!characters.length)
        {
            return -1;
        }

        const value =
            Number.parseInt(
                localStorage.getItem(
                    CURRENT_CHARACTER_KEY) ||
                "0",
                10);

        if (!Number.isInteger(value) ||
            value < 0 ||
            value >= characters.length)
        {
            return 0;
        }

        return value;
    }


    function current()
    {
        const characters =
            load();

        const index =
            currentIndex();

        if (index < 0 ||
            index >= characters.length)
        {
            return null;
        }

        return characters[
            index];
    }


    function select(
        index)
    {
        const characters =
            load();

        if (!Number.isInteger(index) ||
            index < 0 ||
            index >= characters.length)
        {
            return false;
        }

        localStorage.setItem(
            CURRENT_CHARACTER_KEY,
            String(
                index));

        return true;
    }


    function partId(
        choiceGroup,
        value)
    {
        if (!value ||
            typeof value !==
                "object")
        {
            return 0;
        }

        if (choiceGroup ===
            "01_head")
        {
            return Number(
                value.head_id ||
                0);
        }

        return Number(
            value.item_type_ID ||
            0);
    }


    function appearanceEntries(
        appearance)
    {
        if (!appearance ||
            typeof appearance !==
                "object")
        {
            return [];
        }

        if (Array.isArray(
                appearance.random))
        {
            return appearance.random;
        }

        const result =
            [];

        for (const [
                 choiceGroup,
                 value
             ] of Object.entries(
                 appearance))
        {
            if (!choiceGroup ||
                choiceGroup ===
                    "random" ||
                !value ||
                typeof value !==
                    "object")
            {
                continue;
            }

            result.push(
                {
                    choiceGroup:
                        choiceGroup,

                    var:
                        value
                });
        }

        return result;
    }


    function appearanceFields(
        character = current())
    {
        if (!character)
        {
            return [];
        }

        const fields =
            [];

        const entries =
            appearanceEntries(
                character.appearance);

        for (const entry of entries)
        {
            if (!entry ||
                !entry.choiceGroup ||
                !entry.var)
            {
                continue;
            }

            const id =
                partId(
                    entry.choiceGroup,
                    entry.var);

            if (!Number.isFinite(id) ||
                id <= 0)
            {
                continue;
            }

            fields.push(
                entry.choiceGroup);

            fields.push(
                id);
        }

        return fields;
    }


    window.CharacterStore =
    {
        load:
            load,

        save:
            save,

        currentIndex:
            currentIndex,

        current:
            current,

        select:
            select,

        appearanceFields:
            appearanceFields
    };
})();