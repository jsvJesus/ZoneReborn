"use strict";

(() =>
{
    const ROOT =
        "https://zone.local";

    const STARTUP_IMAGE =
        ROOT +
        "/packs/res/soGUI/maps/loadingScreen/appStart3.tga";

    const VERSION_RESOURCE =
        ROOT +
        "/packs/res/scripts/common/VersionSO.pyc";

    const REMEMBERED_LOGIN =
        ROOT +
        "/user/login.cfg";

    const STARTUP_DURATION =
        1400;


    let currentLocale =
        normalizeLocale(
            localStorage.getItem(
                "frontend.locale") ||
            "russian");


    const state =
    {
        accountLogin:
            "",

        currentScreen:
            ""
    };


    const screens =
        new Map();


    function normalizeLocale(
        value)
    {
        const locale =
            String(
                value ||
                "")
                .toLowerCase();

        if (locale === "english" ||
            locale === "russian" ||
            locale === "chinese")
        {
            return locale;
        }

        return "russian";
    }


    function post(
        command,
        ...values)
    {
        if (!window.chrome ||
            !window.chrome.webview)
        {
            return;
        }

        const encoded =
            values.map(
                value =>
                    encodeURIComponent(
                        String(
                            value ??
                            "")));

        window.chrome.webview.postMessage(
            [
                command,
                ...encoded
            ].join("\t"));
    }


    function trace(
        message)
    {
        post(
            "trace",
            message);
    }


    function delay(
        milliseconds)
    {
        return new Promise(
            resolve =>
                setTimeout(
                    resolve,
                    milliseconds));
    }


    async function readRememberedLogin()
    {
        try
        {
            const response =
                await fetch(
                    REMEMBERED_LOGIN,
                    {
                        cache:
                            "no-store"
                    });

            if (!response.ok)
            {
                return "";
            }

            return (
                await response.text()
            ).trim();
        }
        catch
        {
            return "";
        }
    }


    async function readVersion()
    {
        try
        {
            const response =
                await fetch(
                    VERSION_RESOURCE,
                    {
                        cache:
                            "no-store"
                    });

            if (!response.ok)
            {
                return "ver dev";
            }

            const bytes =
                new Uint8Array(
                    await response.arrayBuffer());

            for (let index = 0;
                 index + 4 < bytes.length;
                 ++index)
            {
                if (bytes[index] !== 0x76 ||
                    bytes[index + 1] !== 0x65 ||
                    bytes[index + 2] !== 0x72 ||
                    bytes[index + 3] !== 0x20)
                {
                    continue;
                }

                let result =
                    "";

                for (let cursor = index;
                     cursor < bytes.length &&
                     result.length < 32;
                     ++cursor)
                {
                    const value =
                        bytes[cursor];

                    if (value < 32 ||
                        value > 126)
                    {
                        break;
                    }

                    result +=
                        String.fromCharCode(
                            value);
                }

                if (result.length)
                {
                    return result;
                }
            }
        }
        catch (error)
        {
            trace(
                "Version read failed: " +
                error);
        }

        return "ver dev";
    }


    async function drawStartupTga()
    {
        const canvas =
            document.getElementById(
                "startupCanvas");

        const response =
            await fetch(
                STARTUP_IMAGE,
                {
                    cache:
                        "no-store"
                });

        if (!response.ok)
        {
            throw new Error(
                "Unable to load startup image.");
        }

        const bytes =
            new Uint8Array(
                await response.arrayBuffer());

        if (bytes.length < 18)
        {
            throw new Error(
                "Invalid TGA header.");
        }

        const idLength =
            bytes[0];

        const colorMapType =
            bytes[1];

        const imageType =
            bytes[2];

        const width =
            bytes[12] |
            (bytes[13] << 8);

        const height =
            bytes[14] |
            (bytes[15] << 8);

        const bitsPerPixel =
            bytes[16];

        const descriptor =
            bytes[17];

        if (colorMapType !== 0)
        {
            throw new Error(
                "Color mapped TGA is unsupported.");
        }

        if (imageType !== 2 &&
            imageType !== 3)
        {
            throw new Error(
                "Unsupported TGA image type.");
        }

        if (bitsPerPixel !== 8 &&
            bitsPerPixel !== 24 &&
            bitsPerPixel !== 32)
        {
            throw new Error(
                "Unsupported TGA pixel format.");
        }

        const bytesPerPixel =
            bitsPerPixel /
            8;

        let source =
            18 +
            idLength;

        const topOrigin =
            (descriptor & 0x20) !== 0;

        canvas.width =
            width;

        canvas.height =
            height;

        const context =
            canvas.getContext(
                "2d");

        const image =
            context.createImageData(
                width,
                height);

        for (let sourceY = 0;
             sourceY < height;
             ++sourceY)
        {
            const destinationY =
                topOrigin
                    ? sourceY
                    : height -
                      sourceY -
                      1;

            for (let x = 0;
                 x < width;
                 ++x)
            {
                const destination =
                    (
                        destinationY *
                        width +
                        x
                    ) *
                    4;

                if (imageType === 3)
                {
                    const value =
                        bytes[source++];

                    image.data[
                        destination] =
                            value;

                    image.data[
                        destination + 1] =
                            value;

                    image.data[
                        destination + 2] =
                            value;

                    image.data[
                        destination + 3] =
                            255;

                    continue;
                }

                const blue =
                    bytes[source++];

                const green =
                    bytes[source++];

                const red =
                    bytes[source++];

                const alpha =
                    bytesPerPixel === 4
                        ? bytes[source++]
                        : 255;

                image.data[
                    destination] =
                        red;

                image.data[
                    destination + 1] =
                        green;

                image.data[
                    destination + 2] =
                        blue;

                image.data[
                    destination + 3] =
                        alpha;
            }
        }

        context.putImageData(
            image,
            0,
            0);
    }


    const Router =
    {
        async show(
            name)
        {
            const screen =
                screens.get(
                    name);

            if (!screen)
            {
                throw new Error(
                    "Frontend screen is not registered: " +
                    name);
            }

            const response =
                await fetch(
                    screen.template,
                    {
                        cache:
                            "no-store"
                    });

            if (!response.ok)
            {
                throw new Error(
                    "Unable to load frontend screen: " +
                    name);
            }

            const root =
                document.getElementById(
                    "screenRoot");

            root.innerHTML =
                await response.text();

            state.currentScreen =
                name;

            if (typeof screen.mount ===
                "function")
            {
                await screen.mount(
                    root);
            }
        },


        async reload()
        {
            if (!state.currentScreen)
            {
                return;
            }

            await this.show(
                state.currentScreen);
        }
    };


    const Frontend =
    {
        ROOT:
            ROOT,

        state:
            state,

        post:
            post,

        trace:
            trace,

        readRememberedLogin:
            readRememberedLogin,

        readVersion:
            readVersion,


        getLocale()
        {
            return currentLocale;
        },


        async setLocale(
            locale)
        {
            currentLocale =
                normalizeLocale(
                    locale);

            localStorage.setItem(
                "frontend.locale",
                currentLocale);

            await Router.reload();
        },


        login(
            login,
            password,
            remember)
        {
            state.accountLogin =
                String(
                    login ||
                    "");

            post(
                "login",
                login,
                password,
                remember
                    ? "1"
                    : "0");
        },


        quit()
        {
            post(
                "quit");
        },


        play()
        {
            post(
                "play");
        },
		
		createCharacter(
            name)
        {
            post(
                "character_create",
                name);
        },


        deleteCharacter()
        {
            post(
                "character_delete");
        },


        openUrl(
            url)
        {
            post(
                "open_url",
                url);
        },


        playSound(
            name)
        {
            if (!name)
            {
                return;
            }

            post(
                "ui_sound",
                name);
        },


        showCharacter()
        {
            post(
                "dummy_show");
        },


        hideCharacter()
        {
            post(
                "dummy_hide");
        },


        setCharacterPart(
            group,
            partId)
        {
            post(
                "dummy_part",
                group,
                partId);
        },


        setCharacterFull(
            fields)
        {
            if (!Array.isArray(fields) ||
                fields.length === 0)
            {
                return;
            }

            post(
                "dummy_full",
                ...fields);
        },


        rotateCharacter(
            deltaX,
            deltaY)
        {
            post(
                "dummy_rotate",
                deltaX,
                deltaY);
        }
    };


    window.Frontend =
        Frontend;

    window.FrontendRouter =
        Router;

    window.FrontendScreens =
    {
        register(
            name,
            screen)
        {
            screens.set(
                name,
                screen);
        },

        get(
            name)
        {
            return screens.get(
                name);
        }
    };


    window.ZoneFrontend =
    {
        loginError(
            message)
        {
            const screen =
                screens.get(
                    "login");

            if (screen &&
                typeof screen.loginError ===
                    "function")
            {
                screen.loginError(
                    message);
            }
        },


        async loginComplete(
			character)
		{
			if (window.CharacterStore)
			{
				CharacterStore.set(
					character);
			}

			await Router.show(
				"main");
		},
		
		
		characterCreateResult(
			success,
			message,
			character)
		{
			const screen =
				screens.get(
					state.currentScreen);

			if (screen &&
				typeof screen.characterCreateResult ===
					"function")
			{
				screen.characterCreateResult(
					success,
					message,
					character);
			}
		},
		
		
		characterDeleteResult(
			success,
			message)
		{
			const screen =
				screens.get(
					state.currentScreen);

			if (screen &&
				typeof screen.characterDeleteResult ===
					"function")
			{
				screen.characterDeleteResult(
					success,
					message);
			}
		},


        localizationResult(
            data)
        {
            window.dispatchEvent(
                new CustomEvent(
                    "frontend-localization",
                    {
                        detail:
                            data
                    }));
        },


        keybindLocalizeResult(
            data)
        {
            window.dispatchEvent(
                new CustomEvent(
                    "frontend-keybinds",
                    {
                        detail:
                            data
                    }));
        }
    };


    async function boot()
    {
        try
        {
            await Promise.all([
                drawStartupTga(),
                delay(
                    STARTUP_DURATION)
            ]);
        }
        catch (error)
        {
            trace(
                "Startup image failed: " +
                error);
        }

        const startup =
            document.getElementById(
                "startup");

        startup.style.display =
            "none";

        await Router.show(
            "login");

        post(
            "ready");

        trace(
            "HTML frontend ready.");
    }


    window.addEventListener(
        "error",
        event =>
        {
            trace(
                "JS error: " +
                event.message);
        });


    window.addEventListener(
        "unhandledrejection",
        event =>
        {
            trace(
                "Promise rejection: " +
                event.reason);
        });


    window.addEventListener(
        "DOMContentLoaded",
        () =>
        {
            boot();
        });
})();