"use strict";

(() =>
{
    let active =
        null;


    function close()
    {
        if (!active)
        {
            return;
        }

        active.overlay.remove();

        active =
            null;
    }


    function setBusy(
        value)
    {
        if (!active)
        {
            return;
        }

        active.busy =
            Boolean(
                value);

        active.confirm.disabled =
            active.busy;

        active.cancel.disabled =
            active.busy;

        if (active.input)
        {
            active.input.disabled =
                active.busy;
        }

        active.overlay.classList.toggle(
            "busy",
            active.busy);
    }


    function fail(
        message)
    {
        if (!active)
        {
            return;
        }

        setBusy(
            false);

        active.error.textContent =
            String(
                message ||
                "");

        active.error.hidden =
            !active.error.textContent;
    }


    function createBase(
        options)
    {
        close();

        const overlay =
            document.createElement(
                "div");

        overlay.className =
            "character-dialog-overlay";


        const windowElement =
            document.createElement(
                "div");

        windowElement.className =
            "character-dialog-window";


        const title =
            document.createElement(
                "div");

        title.className =
            "character-dialog-title";

        title.textContent =
            String(
                options.title ||
                "");


        const line =
            document.createElement(
                "div");

        line.className =
            "character-dialog-title-line";


        const message =
            document.createElement(
                "div");

        message.className =
            "character-dialog-message";

        message.textContent =
            String(
                options.message ||
                "");

        message.hidden =
            !message.textContent;


        const content =
            document.createElement(
                "div");

        content.className =
            "character-dialog-content";


        const error =
            document.createElement(
                "div");

        error.className =
            "character-dialog-error";

        error.hidden =
            true;


        const buttons =
            document.createElement(
                "div");

        buttons.className =
            "character-dialog-buttons";


        const cancel =
            document.createElement(
                "button");

        cancel.type =
            "button";

        cancel.className =
            "character-dialog-button secondary";

        cancel.textContent =
            String(
                options.cancelText ||
                "Cancel");


        const confirm =
            document.createElement(
                "button");

        confirm.type =
            "button";

        confirm.className =
            "character-dialog-button primary";

        confirm.textContent =
            String(
                options.confirmText ||
                "OK");


        buttons.appendChild(
            cancel);

        buttons.appendChild(
            confirm);


        windowElement.appendChild(
            title);

        windowElement.appendChild(
            line);

        windowElement.appendChild(
            message);

        windowElement.appendChild(
            content);

        windowElement.appendChild(
            error);

        windowElement.appendChild(
            buttons);

        overlay.appendChild(
            windowElement);

        document.body.appendChild(
            overlay);


        active =
        {
            overlay:
                overlay,

            window:
                windowElement,

            content:
                content,

            error:
                error,

            cancel:
                cancel,

            confirm:
                confirm,

            input:
                null,

            busy:
                false
        };


        cancel.addEventListener(
            "click",
            () =>
            {
                if (!active ||
                    active.busy)
                {
                    return;
                }

                close();
            });


        overlay.addEventListener(
            "pointerdown",
            event =>
            {
                if (!active ||
                    active.busy)
                {
                    return;
                }

                if (event.target ===
                    overlay)
                {
                    close();
                }
            });


        return active;
    }


    function openCreate(
        options)
    {
        const dialog =
            createBase(
                options);


        const label =
            document.createElement(
                "div");

        label.className =
            "character-dialog-label";

        label.textContent =
            String(
                options.inputLabel ||
                "");


        const input =
            document.createElement(
                "input");

        input.type =
            "text";

        input.className =
            "character-dialog-input";

        input.maxLength =
            64;

        input.autocomplete =
            "off";

        input.spellcheck =
            false;

        input.placeholder =
            String(
                options.placeholder ||
                "");


        dialog.content.appendChild(
            label);

        dialog.content.appendChild(
            input);

        dialog.input =
            input;


        const submit =
            () =>
            {
                if (!active ||
                    active.busy)
                {
                    return;
                }

                const value =
                    input.value.trim();

                if (typeof options.validate ===
                    "function")
                {
                    const validationError =
                        options.validate(
                            value);

                    if (validationError)
                    {
                        fail(
                            validationError);

                        return;
                    }
                }

                active.error.hidden =
                    true;

                active.error.textContent =
                    "";

                setBusy(
                    true);

                if (typeof options.onConfirm ===
                    "function")
                {
                    options.onConfirm(
                        value);
                }
            };


        dialog.confirm.addEventListener(
            "click",
            submit);


        input.addEventListener(
            "keydown",
            event =>
            {
                if (event.key ===
                    "Enter")
                {
                    event.preventDefault();

                    submit();
                }
            });


        window.setTimeout(
            () =>
            {
                if (input.isConnected)
                {
                    input.focus();
                }
            },
            0);
    }


    function openConfirm(
        options)
    {
        const dialog =
            createBase(
                options);

        dialog.confirm.classList.add(
            "danger");

        dialog.confirm.addEventListener(
            "click",
            () =>
            {
                if (!active ||
                    active.busy)
                {
                    return;
                }

                setBusy(
                    true);

                if (typeof options.onConfirm ===
                    "function")
                {
                    options.onConfirm();
                }
            });
    }


    window.CharacterDialog =
    {
        openCreate:
            openCreate,

        openConfirm:
            openConfirm,

        fail:
            fail,

        close:
            close,

        setBusy:
            setBusy
    };
})();