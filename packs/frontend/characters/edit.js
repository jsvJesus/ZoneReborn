"use strict";

(() =>
{
    const CONFIG_PATH =
        "/packs/res/scripts/common/data/charMakerCfg.json";

    const CLOTHING_COLOURS =
        Object.freeze(
            [
                0xFFFFFF, 0xD8D3C9, 0xB8AE9E, 0x918675,
                0x6F675B, 0x555555, 0x2C2C2C, 0x171717,
                0x33210E, 0x3C3123, 0x654832, 0xA86536,
                0xCC9862, 0xE4CE93, 0xAA9464, 0x85806D,
                0x66664E, 0x4C5339, 0x36442F, 0x29382D,
                0x657A61, 0x718B82, 0x617B7A, 0x4E696A,
                0x405859, 0x354C4E, 0x2B3F40, 0x223333
            ]);

    const translations =
    {
        russian:
        {
            title: "\u041C\u041E\u0414\u0418\u0424\u0418\u041A\u0410\u0426\u0418\u042F \u0412\u041D\u0415\u0428\u041D\u041E\u0421\u0422\u0418",
            appearance: "\u0412\u042B\u0411\u041E\u0420 \u0412\u041D\u0415\u0428\u041D\u041E\u0421\u0422\u0418",
            clothes: "\u0412\u042B\u0411\u041E\u0420 \u041E\u0414\u0415\u0416\u0414\u042B",
            reset: "\u0421\u0411\u0420\u041E\u0421\u0418\u0422\u042C",
            random: "\u0421\u041B\u0423\u0427\u0410\u0419\u041D\u041E",
            apply: "\u041F\u0420\u0418\u041C\u0415\u041D\u0418\u0422\u042C",
            back: "\u041D\u0410\u0417\u0410\u0414",
            configError: "\u041D\u0435 \u0443\u0434\u0430\u043B\u043E\u0441\u044C \u0437\u0430\u0433\u0440\u0443\u0437\u0438\u0442\u044C \u043D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u043E\u0434\u0435\u0436\u0434\u044B."
        },
        english:
        {
            title: "APPEARANCE MODIFICATION",
            appearance: "CHOOSE APPEARANCE",
            clothes: "CHOOSE CLOTHES",
            reset: "RESET",
            random: "RANDOM",
            apply: "APPLY",
            back: "BACK",
            configError: "Unable to load clothing settings."
        },
        chinese:
        {
            title: "\u5916\u89C2\u4FEE\u6539",
            appearance: "\u9009\u62E9\u5916\u89C2",
            clothes: "\u9009\u62E9\u670D\u88C5",
            reset: "\u91CD\u7F6E",
            random: "\u968F\u673A",
            apply: "\u5E94\u7528",
            back: "\u8FD4\u56DE",
            configError: "\u65E0\u6CD5\u52A0\u8F7D\u670D\u88C5\u8BBE\u7F6E\u3002"
        }
    };

    let root = null;
    let groups = [];
    let selection = [];
    let sessionActive = false;
    let ready = false;
    let busy = false;

    function element(id)
    {
        return root?.querySelector("#" + id) || null;
    }

    function text(key)
    {
        const table = translations[Frontend.getLocale()] || translations.russian;
        return table[key] || translations.russian[key] || key;
    }

    function fieldsFor(values)
    {
        return values.flatMap(value => [value.group, value.itemType, value.colour]);
    }

    function defaultSelection()
    {
        return groups.map(group =>
            ({ group: group.name, itemType: group.options[0].itemType, colour: 0xFFFFFF }));
    }

    function normalizeSelection(values)
    {
        const source = Array.isArray(values) ? values : [];

        return groups.map(group =>
        {
            const saved = source.find(value => value?.group === group.name);
            const itemType = Number(saved?.itemType);
            const validItem = group.options.some(option => option.itemType === itemType);
            const colour = Number(saved?.colour);

            return {
                group: group.name,
                itemType: validItem ? itemType : group.options[0].itemType,
                colour: Number.isInteger(colour) && colour >= 0 && colour <= 0xFFFFFF
                    ? colour
                    : 0xFFFFFF
            };
        });
    }

    function randomSelection()
    {
        return groups.map(group =>
        {
            const option = group.options[Math.floor(Math.random() * group.options.length)];
            const colour = CLOTHING_COLOURS[Math.floor(Math.random() * CLOTHING_COLOURS.length)];
            return { group: group.name, itemType: option.itemType, colour: colour };
        });
    }

    async function loadGroups()
    {
        const response = await fetch(CONFIG_PATH, { cache: "no-store" });
        if (!response.ok) throw new Error("HTTP " + response.status);
        const config = await response.json();

        groups = Object.entries(config).map(([name, options]) =>
            ({
                name: name,
                options: Array.isArray(options)
                    ? options.map(option =>
                        ({
                            itemType: Number(option.item_id),
                            texture: String(option.texture || ""),
                            caption: option.caption || option.name || ""
                        })).filter(option => Number.isFinite(option.itemType) && option.itemType > 0 && option.texture)
                    : []
            })).filter(group => group.options.length > 0);

        if (!groups.length) throw new Error("Character editor config is empty.");
    }

    function showError(message)
    {
        const error = element("editorError");
        error.textContent = String(message || "");
        error.hidden = !error.textContent;
    }

    function setBusy(value)
    {
        busy = Boolean(value);
        ["editorAppearance", "editorClothes", "editorReset", "editorRandom", "editorApply", "editorBack"]
            .forEach(id =>
            {
                const button = element(id);
                if (button) button.disabled = busy || (!ready && id !== "editorBack");
            });
    }

    function resetEditor()
    {
        if (!ready || busy) return;
        selection = defaultSelection();
        CharacterClothes.setSelection(selection);
        showError("");
        Frontend.resetCharacterEditor();
    }

    function randomizeEditor()
    {
        if (!ready || busy) return;
        selection = randomSelection();
        CharacterClothes.setSelection(selection);
        showError("");
        Frontend.randomizeCharacterEditor(fieldsFor(selection));
    }

    function resetClothes()
    {
        if (!ready || busy) return;
        selection = defaultSelection();
        CharacterClothes.setSelection(selection);
        Frontend.setCharacterFull(fieldsFor(selection));
    }

    function randomizeClothes()
    {
        if (!ready || busy) return;
        selection = randomSelection();
        CharacterClothes.setSelection(selection);
        Frontend.setCharacterFull(fieldsFor(selection));
    }

    function closeClothes()
    {
        if (ready) Frontend.setCharacterFull(fieldsFor(selection));
    }

    function selectPart(groupName, itemType)
    {
        if (!ready || busy) return;
        const group = groups.find(value => value.name === groupName);
        const selected = selection.find(value => value.group === groupName);
        const value = Number(itemType);
        if (!group || !selected || !group.options.some(option => option.itemType === value) || selected.itemType === value) return;

        selected.itemType = value;
        CharacterClothes.setSelection(selection);
        Frontend.setCharacterPart(groupName, selected.itemType, selected.colour);
    }

    function selectColour(groupName, colour)
    {
        if (!ready || busy) return;
        const selected = selection.find(value => value.group === groupName);
        const value = Number(colour);
        if (!selected || !CLOTHING_COLOURS.includes(value) || selected.colour === value) return;

        selected.colour = value;
        CharacterClothes.setSelection(selection);
        Frontend.setCharacterPart(groupName, selected.itemType, selected.colour);
    }

    function applyEditor()
    {
        if (!ready || busy) return;
        showError("");
        setBusy(true);
        Frontend.applyCharacterEditor();
    }

    async function leaveEditor()
    {
        if (busy) return;
        sessionActive = false;
        selection = [];
        Frontend.cancelCharacterEditor();
        await FrontendRouter.show("main");
    }

    function bindRotation()
    {
        const area = element("editorRotateArea");
        let pointer = -1;
        let lastX = 0;
        let lastY = 0;

        area.addEventListener("contextmenu", event => event.preventDefault());
        area.addEventListener("pointerdown", event =>
        {
            if (event.button !== 2) return;
            event.preventDefault();
            pointer = event.pointerId;
            lastX = event.clientX;
            lastY = event.clientY;
            area.classList.add("rotating");
            area.setPointerCapture(pointer);
        });
        area.addEventListener("pointermove", event =>
        {
            if (event.pointerId !== pointer) return;
            const deltaX = event.clientX - lastX;
            const deltaY = event.clientY - lastY;
            lastX = event.clientX;
            lastY = event.clientY;
            Frontend.rotateCharacter(deltaX, deltaY);
        });
        const stop = event =>
        {
            if (event.pointerId !== pointer) return;
            pointer = -1;
            area.classList.remove("rotating");
        };
        area.addEventListener("pointerup", stop);
        area.addEventListener("pointercancel", stop);
        area.addEventListener("lostpointercapture", () =>
        {
            pointer = -1;
            area.classList.remove("rotating");
        });
    }

    function bindActions()
    {
        element("editorAppearance").addEventListener("click", () => CharacterAppearance.open());
        element("editorClothes").addEventListener("click", () => CharacterClothes.open());
        element("editorReset").addEventListener("click", resetEditor);
        element("editorRandom").addEventListener("click", randomizeEditor);
        element("editorApply").addEventListener("click", applyEditor);
        element("editorBack").addEventListener("click", leaveEditor);
    }

    async function mount(container)
    {
        const character = Frontend.currentCharacter();
        if (!character)
        {
            await FrontendRouter.show("main");
            return;
        }

        root = container;
        groups = [];
        ready = false;
        busy = false;

        root.querySelectorAll("[data-editor-i18n]").forEach(node =>
        {
            node.textContent = text(node.dataset.editorI18n);
        });

        element("editorVersion").textContent = await Frontend.readVersion();
        bindActions();
        bindRotation();

        await CharacterAppearance.initialize(root);
        CharacterClothes.initialize(root,
        {
            select: selectPart,
            selectColour: selectColour,
            reset: resetClothes,
            random: randomizeClothes,
            close: closeClothes
        });

        if (!sessionActive)
        {
            selection = [];
            sessionActive = true;
        }

        Frontend.openCharacterEditor();

        setBusy(false);

        try
        {
            await loadGroups();
            selection = normalizeSelection(selection.length ? selection : character.appearance);
            CharacterClothes.setData(groups, selection, CLOTHING_COLOURS);
            ready = true;
            setBusy(false);
        }
        catch (error)
        {
            Frontend.trace("Character editor config failed: " + error);
            showError(text("configError"));
            setBusy(false);
        }
    }

    async function characterEditResult(success, message)
    {
        if (!success)
        {
            setBusy(false);
            showError(message);
            return;
        }

        sessionActive = false;
        selection = [];
        await FrontendRouter.show("main");
    }

    function characterFaceState(face)
    {
        CharacterAppearance.receiveState(face);
    }

    FrontendScreens.register("character-edit",
    {
        template: "./characters/edit.html",
        mount: mount,
        characterEditResult: characterEditResult,
        characterFaceState: characterFaceState
    });
})();
