"use strict";

(() =>
{
    const CONFIG_PATH =
        "/packs/res/scripts/common/data/charMakerCfg.json";


    const translations =
    {
        russian:
        {
            mainTitle:
                "ГЛАВНОЕ МЕНЮ",

            name:
                "ИМЯ:",

            appearance:
                "ВЫБОР ВНЕШНОСТИ",

            clothes:
                "ВЫБОР ОДЕЖДЫ",

            reset:
                "СБРОСИТЬ",

            random:
                "СЛУЧАЙНО",

            create:
                "СОЗДАТЬ",

            back:
                "НАЗАД",

            invalidName:
                "Имя должно содержать от 3 до 32 символов: буквы, цифры и не более одного _",

            configError:
                "Не удалось загрузить настройки внешности."
        },


        english:
        {
            mainTitle:
                "MAIN MENU",

            name:
                "NAME:",

            appearance:
                "CHOOSE APPEARANCE",

            clothes:
                "CHOOSE CLOTHES",

            reset:
                "RESET",

            random:
                "RANDOM",

            create:
                "CREATE",

            back:
                "BACK",

            invalidName:
                "Name must contain 3 to 32 characters: letters, numbers and no more than one _",

            configError:
                "Unable to load character appearance settings."
        },


        chinese:
        {
            mainTitle:
                "主菜单",

            name:
                "名称：",

            appearance:
                "选择外观",

            clothes:
                "选择服装",

            reset:
                "重置",

            random:
                "随机",

            create:
                "创建",

            back:
                "返回",

            invalidName:
                "名称必须包含 3 到 32 个字符",

            configError:
                "无法加载角色外观设置。"
        }
    };


    let mountedRoot =
        null;

    let groups =
        [];

    let selection =
        [];

    let busy =
        false;

    let ready =
        false;


    function element(
        id)
    {
        return mountedRoot
            ? mountedRoot.querySelector(
                "#" +
                id)
            : null;
    }


    function translation(
        key)
    {
        const table =
            translations[
                Frontend.getLocale()] ||
            translations.russian;

        return table[key] ||
            translations.russian[key] ||
            key;
    }


    function applyTranslations()
    {
        mountedRoot
            .querySelectorAll(
                "[data-i18n]")
            .forEach(
                node =>
                {
                    node.textContent =
                        translation(
                            node.dataset.i18n);
                });

        mountedRoot
            .querySelectorAll(
                "[data-locale]")
            .forEach(
                button =>
                {
                    button.classList.toggle(
                        "selected",
                        button.dataset.locale ===
                            Frontend.getLocale());
                });
    }


    function formatBalance(
        value)
    {
        const number =
            Number(
                value);

        if (!Number.isFinite(
                number))
        {
            return "0";
        }

        let locale =
            "ru-RU";

        if (Frontend.getLocale() ===
            "english")
        {
            locale =
                "en-US";
        }
        else if (Frontend.getLocale() ===
                 "chinese")
        {
            locale =
                "zh-CN";
        }

        return new Intl.NumberFormat(
            locale,
            {
                maximumFractionDigits:
                    0
            }).format(
                Math.max(
                    0,
                    number));
    }


    function validateName(
        value)
    {
        const characters =
            Array.from(
                value);

        if (characters.length < 3 ||
            characters.length > 32 ||
            !/^[A-Za-z0-9_\u0400-\u052F]+$/u.test(
                value))
        {
            return false;
        }

        const underscores =
            characters.filter(
                character =>
                    character === "_")
                .length;

        return underscores <= 1 &&
            !value.startsWith(
                "_") &&
            !value.endsWith(
                "_");
    }


    function showError(
        message)
    {
        const error =
            element(
                "creatorError");

        error.textContent =
            String(
                message ||
                "");

        error.hidden =
            !error.textContent;
    }


    function updateSubmit()
    {
        const input =
            element(
                "creatorName");

        element(
            "creatorSubmit")
            .disabled =
                busy ||
                !ready ||
                !validateName(
                    input.value.trim());
    }


    function setBusy(
        value)
    {
        busy =
            Boolean(
                value);

        element(
            "creatorName")
            .disabled =
                busy;

        element(
            "creatorReset")
            .disabled =
                busy;

        element(
            "creatorRandom")
            .disabled =
                busy ||
                !ready;

        element(
            "creatorBack")
            .disabled =
                busy;

        element(
            "creatorAppearance")
            .disabled =
                busy;

        updateSubmit();
    }


    function fieldsFor(
        values)
    {
        const fields =
            [];

        for (const value of values)
        {
            fields.push(
                value.group,
                value.itemType);
        }

        return fields;
    }


    function defaultSelection()
    {
        return groups.map(
            group =>
                ({
                    group:
                        group.name,

                    itemType:
                        group.options[0]
                }));
    }


    function randomSelection()
    {
        return groups.map(
            group =>
            {
                const index =
                    Math.floor(
                        Math.random() *
                        group.options.length);

                return {
                    group:
                        group.name,

                    itemType:
                        group.options[index]
                };
            });
    }


    async function loadGroups()
    {
        const response =
            await fetch(
                CONFIG_PATH,
                {
                    cache:
                        "no-store"
                });

        if (!response.ok)
        {
            throw new Error(
                "HTTP " +
                response.status);
        }

        const config =
            await response.json();

        groups =
            Object.entries(
                config)
                .map(
                    ([name, options]) =>
                        ({
                            name:
                                name,

                            options:
                                Array.isArray(options)
                                    ? options
                                        .map(
                                            option =>
                                                Number(
                                                    option.item_id))
                                        .filter(
                                            itemType =>
                                                Number.isFinite(
                                                    itemType) &&
                                                itemType > 0)
                                    : []
                        }))
                .filter(
                    group =>
                        group.options.length > 0);

        if (!groups.length)
        {
            throw new Error(
                "Character creator config is empty.");
        }
    }


    function resetCreator()
    {
        selection =
            defaultSelection();

        showError(
            "");

        Frontend.resetCharacterCreator();
    }


    function randomizeCreator()
    {
        selection =
            randomSelection();

        showError(
            "");

        Frontend.setCharacterFull(
            fieldsFor(
                selection));
    }


    function submitCreator()
    {
        const name =
            element(
                "creatorName")
                .value
                .trim();

        if (!validateName(
                name))
        {
            showError(
                translation(
                    "invalidName"));

            return;
        }

        showError(
            "");

        setBusy(
            true);

        Frontend.createCharacter(
            name,
            fieldsFor(
                selection));
    }


    async function leaveCreator()
    {
        if (busy)
        {
            return;
        }

        Frontend.hideCharacter();

        await FrontendRouter.show(
            "main");
    }


    function bindRotation()
    {
        const area =
            element(
                "creatorRotateArea");

        let pointer =
            -1;

        let lastX =
            0;

        let lastY =
            0;

        area.addEventListener(
            "contextmenu",
            event =>
                event.preventDefault());

        area.addEventListener(
            "pointerdown",
            event =>
            {
                if (event.button !== 2)
                {
                    return;
                }

                event.preventDefault();

                pointer =
                    event.pointerId;

                lastX =
                    event.clientX;

                lastY =
                    event.clientY;

                area.classList.add(
                    "rotating");

                area.setPointerCapture(
                    pointer);
            });

        area.addEventListener(
            "pointermove",
            event =>
            {
                if (event.pointerId !== pointer)
                {
                    return;
                }

                const deltaX =
                    event.clientX -
                    lastX;

                const deltaY =
                    event.clientY -
                    lastY;

                lastX =
                    event.clientX;

                lastY =
                    event.clientY;

                Frontend.rotateCharacter(
                    deltaX,
                    deltaY);
            });

        const stop =
            event =>
            {
                if (event.pointerId !== pointer)
                {
                    return;
                }

                pointer =
                    -1;

                area.classList.remove(
                    "rotating");
            };

        area.addEventListener(
            "pointerup",
            stop);

        area.addEventListener(
            "pointercancel",
            stop);

        area.addEventListener(
            "lostpointercapture",
            () =>
            {
                pointer =
                    -1;

                area.classList.remove(
                    "rotating");
            });
    }


    function bindLocales()
    {
        mountedRoot
            .querySelectorAll(
                "[data-locale]")
            .forEach(
                button =>
                {
                    button.addEventListener(
                        "click",
                        async () =>
                        {
                            if (button.dataset.locale ===
                                Frontend.getLocale())
                            {
                                return;
                            }

                            await Frontend.setLocale(
                                button.dataset.locale);
                        });
                });
    }


    function bindActions()
    {
        const input =
            element(
                "creatorName");

        input.addEventListener(
            "input",
            () =>
            {
                showError(
                    "");

                updateSubmit();
            });

        input.addEventListener(
            "keydown",
            event =>
            {
                if (event.key === "Enter" &&
                    !element(
                        "creatorSubmit")
                        .disabled)
                {
                    event.preventDefault();

                    submitCreator();
                }
            });

        element(
            "creatorReset")
            .addEventListener(
                "click",
                resetCreator);

        element(
            "creatorRandom")
            .addEventListener(
                "click",
                randomizeCreator);

        element(
            "creatorSubmit")
            .addEventListener(
                "click",
                submitCreator);

        element(
            "creatorBack")
            .addEventListener(
                "click",
                leaveCreator);

        element(
            "creatorAppearance")
            .addEventListener(
                "click",
                () => window.CharacterAppearance.open());

        mountedRoot
            .querySelectorAll(
                ".creator-future")
            .forEach(
                button =>
                {
                    button.addEventListener(
                        "click",
                        () =>
                            Frontend.trace(
                                "Character creator submenu is not implemented yet: " +
                                button
                                    .querySelector(
                                        "[data-i18n]")
                                    .dataset.i18n));
                });
    }


    async function mount(
        root)
    {
        mountedRoot =
            root;

        groups =
            [];

        selection =
            [];

        busy =
            false;

        ready =
            false;

        applyTranslations();

        element(
            "creatorAccountName")
            .textContent =
                Frontend.state.accountLogin ||
                "PLAYER";

        const accountId =
            Frontend.state.accountId;

        element(
            "creatorAccountId")
            .textContent =
                accountId === null ||
                accountId === undefined ||
                accountId === ""
                    ? "ID —"
                    : "ID " +
                      String(
                          accountId);

        element(
            "creatorSoftBalanceValue")
            .textContent =
                formatBalance(
                    Frontend.state.softCurrency ??
                    0);

        element(
            "creatorPremiumBalanceValue")
            .textContent =
                formatBalance(
                    Frontend.state.premiumCurrency ??
                    0);

        if (window.FrontendTga)
        {
            await window.FrontendTga.load(
                element(
                    "creatorPremiumIcon"));
        }

        element(
            "creatorVersion")
            .textContent =
                await Frontend.readVersion();

        bindLocales();
        bindActions();
        bindRotation();

        await window.CharacterAppearance.initialize(
            mountedRoot);

        element(
            "creatorRandom")
            .disabled =
                true;

        Frontend.resetCharacterCreator();

        try
        {
            await loadGroups();

            selection =
                defaultSelection();

            ready =
                true;

            element(
                "creatorRandom")
                .disabled =
                    false;

            updateSubmit();
        }
        catch (error)
        {
            Frontend.trace(
                "Character creator config failed: " +
                error);

            showError(
                translation(
                    "configError"));

            element(
                "creatorRandom")
                .disabled =
                    true;

            element(
                "creatorSubmit")
                .disabled =
                    true;

        }

        window.setTimeout(
            () =>
            {
                const input =
                    element(
                        "creatorName");

                if (input &&
                    input.isConnected)
                {
                    input.focus();
                }
            },
            0);
    }


    async function characterCreateResult(
        success,
        message)
    {
        if (!success)
        {
            setBusy(
                false);

            showError(
                message);

            return;
        }

        await FrontendRouter.show(
            "main");
    }


    function characterFaceState(
        face)
    {
        window.CharacterAppearance.receiveState(
            face);
    }


    FrontendScreens.register(
        "character-create",
        {
            template:
                "./characters/create.html",

            mount:
                mount,

            characterCreateResult:
                characterCreateResult,

            characterFaceState:
                characterFaceState
        });
})();
