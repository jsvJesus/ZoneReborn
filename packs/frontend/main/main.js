"use strict";

(() =>
{
    let mountedRoot =
        null;

    let dragging =
        false;

    let lastMouseX =
        0;

    let lastMouseY =
        0;


    function element(
        id)
    {
        return mountedRoot
            ? mountedRoot.querySelector(
                "#" + id)
            : null;
    }


    function applyCurrentCharacter()
    {
        Frontend.showCharacter();

        const fields =
            CharacterStore.
                appearanceFields();

        if (fields.length)
        {
            Frontend.setCharacterFull(
                fields);
        }
    }


    function renderCharacters()
    {
        const container =
            element(
                "mainCharacterList");

        container.replaceChildren();


        const characters =
            CharacterStore.load();

        const selected =
            CharacterStore.
                currentIndex();


        for (let index = 0;
             index < characters.length &&
             index < 3;
             ++index)
        {
            const character =
                characters[index];

            const button =
                document.createElement(
                    "button");

            button.type =
                "button";

            button.className =
                "main-character";

            if (index ===
                selected)
            {
                button.classList.add(
                    "selected");
            }

            button.textContent =
                String(
                    character.name ||
                    (
                        "Character " +
                        (index + 1)
                    ));

            button.addEventListener(
                "click",
                () =>
                {
                    if (!CharacterStore.
                            select(
                                index))
                    {
                        return;
                    }

                    renderCharacters();

                    applyCurrentCharacter();
                });

            container.appendChild(
                button);
        }


        const maxSlots =
            3;

        for (let index =
                 characters.length;
             index < maxSlots;
             ++index)
        {
            const button =
                document.createElement(
                    "button");

            button.type =
                "button";

            button.className =
                "main-character new";

            button.textContent =
                "НОВЫЙ ПЕРСОНАЖ";

            button.addEventListener(
                "click",
                () =>
                {
                    Frontend.trace(
                        "Open character creation.");
                });

            container.appendChild(
                button);
        }
    }


    function bindCharacterRotation()
    {
        const area =
            element(
                "characterRotateArea");

        area.addEventListener(
            "pointerdown",
            event =>
            {
                dragging =
                    true;

                lastMouseX =
                    event.clientX;

                lastMouseY =
                    event.clientY;

                area.classList.add(
                    "dragging");

                area.setPointerCapture(
                    event.pointerId);
            });


        area.addEventListener(
            "pointermove",
            event =>
            {
                if (!dragging)
                {
                    return;
                }

                const deltaX =
                    event.clientX -
                    lastMouseX;

                const deltaY =
                    event.clientY -
                    lastMouseY;

                lastMouseX =
                    event.clientX;

                lastMouseY =
                    event.clientY;

                Frontend.rotateCharacter(
                    deltaX,
                    deltaY);
            });


        const stop =
            event =>
            {
                if (!dragging)
                {
                    return;
                }

                dragging =
                    false;

                area.classList.remove(
                    "dragging");

                if (area.hasPointerCapture(
                        event.pointerId))
                {
                    area.releasePointerCapture(
                        event.pointerId);
                }
            };

        area.addEventListener(
            "pointerup",
            stop);

        area.addEventListener(
            "pointercancel",
            stop);
    }


    async function mount(
        root)
    {
        mountedRoot =
            root;


        element(
            "mainAccountName").
            textContent =
                Frontend.state.
                    accountLogin ||
                "PLAYER";


        element(
            "mainVersion").
            textContent =
                await Frontend.
                    readVersion();


        root
            .querySelectorAll(
                "[data-locale]")
            .forEach(
                button =>
                {
                    button.classList.toggle(
                        "selected",
                        button.dataset.locale ===
                            Frontend.getLocale());

                    button.addEventListener(
                        "click",
                        () =>
                            Frontend.setLocale(
                                button.dataset.locale));
                });


        element(
            "mainPlay").
            addEventListener(
                "click",
                () =>
                    Frontend.play());


        element(
            "mainExit").
            addEventListener(
                "click",
                () =>
                    Frontend.quit());


        element(
            "mainClose").
            addEventListener(
                "click",
                () =>
                    Frontend.quit());


        element(
            "mainStore").
            addEventListener(
                "click",
                () =>
                    Frontend.trace(
                        "Open store."));


        element(
            "mainStorage").
            addEventListener(
                "click",
                () =>
                    Frontend.trace(
                        "Open storage."));


        root
            .querySelectorAll(
                "[data-action]")
            .forEach(
                button =>
                {
                    button.addEventListener(
                        "click",
                        () =>
                        {
                            Frontend.trace(
                                "Main action: " +
                                button.dataset.action);
                        });
                });


        renderCharacters();

        bindCharacterRotation();

        applyCurrentCharacter();
    }


    FrontendScreens.register(
        "main",
        {
            template:
                "./main/main.html",

            mount:
                mount
        });
})();