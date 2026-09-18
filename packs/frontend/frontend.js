"use strict";

const ROOT =
    "https://zone.local";

const SWF_URL =
    ROOT +
    "/packs/res/extendedGUI/MainMenuGUI.swf";
	
const SWF_BASE =
    ROOT +
    "/packs/res/extendedGUI/";

const STARTUP_IMAGE =
    ROOT +
    "/packs/res/soGUI/maps/loadingScreen/appStart.tga";

const SERVER_CONFIG =
    ROOT +
    "/packs/res/scripts/client/data/servers_config.json";

const LOCALES_CONFIG =
    ROOT +
    "/packs/res/local/localizations.json";

const VERSION_RESOURCE =
    ROOT +
    "/packs/res/scripts/common/VersionSO.pyc";

const REMEMBERED_LOGIN =
    ROOT +
    "/user/login.cfg";

const STARTUP_DURATION =
    1400;

let player =
    null;

let currentLocale =
    normalizeLocale(
        localStorage.getItem(
            "zone.locale") ||
        "russian");


function normalizeLocale(value)
{
    const locale =
        String(
            value ||
            "")
            .toLowerCase();

    return locale ===
        "english"
            ? "english"
            : "russian";
}


function postToHost(
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


function trace(message)
{
    postToHost(
        "trace",
        message);
}


function unwrapArguments(value)
{
    if (!value)
    {
        return {};
    }

    if (Array.isArray(
        value.arguments))
    {
        if (value.arguments.length ===
            1)
        {
            return (
                value.arguments[0] ||
                {}
            );
        }

        return value.arguments;
    }

    if (value.arguments &&
        typeof value.arguments ===
            "object")
    {
        return value.arguments;
    }

    return value;
}


function transmit(
    eventName,
    data = {})
{
	trace(
    "Host -> Flash event=" +
    eventName +
    " data=" +
    safeDebugValue(data));
    if (!player)
    {
        return;
    }

    const message =
        JSON.stringify({
            event_name:
                eventName,

            data:
                data ??
                {}
        });

    //
    // Scaleform:
    // movie.invoke(
    //     "externalInterfaceTransmit",
    //     response)
    //
    // Ruffle:
    // вызываем зарегистрированный
    // ExternalInterface callback.
    //
    try
    {
        if (typeof player.externalInterfaceTransmit ===
            "function")
        {
            player.externalInterfaceTransmit(
                message);

            return;
        }
    }
    catch (error)
    {
        trace(
            "externalInterfaceTransmit direct: " +
            error);
    }

    try
    {
        player
            .ruffle()
            .callExternalInterface(
                "externalInterfaceTransmit",
                message);
    }
    catch (error)
    {
        trace(
            "externalInterfaceTransmit: " +
            error);
    }
}


function transmitAsync(
    eventName,
    data = {})
{
    setTimeout(
        () =>
        {
            transmit(
                eventName,
                data);
        },
        0);
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

            if (result.length !==
                0)
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


async function buildServerList()
{
    const result = {
        list: [],
        defaultServerId: ""
    };

    try
    {
        const response =
            await fetch(
                SERVER_CONFIG,
                {
                    cache:
                        "no-store"
                });

        const config =
            await response.json();

        const groups = [
            {
                name:
                    "customers_servers",

                developer:
                    false
            },

            {
                name:
                    "developers_servers",

                developer:
                    true
            }
        ];

        for (const group of groups)
        {
            const servers =
                Array.isArray(
                    config[group.name])
                    ? config[group.name]
                    : [];

            for (const serverInfo of
                 servers)
            {
                if (!serverInfo ||
                    !serverInfo.server ||
                    !serverInfo.server.online)
                {
                    continue;
                }

                const id =
                    String(
                        result.list.length);

                result.list.push({
                    id:
                        id,

                    label:
                        serverInfo.name ||
                        "",

                    address:
                        serverInfo.server.host ||
                        "",

                    //
                    // Реальный SO получал их
                    // отдельными runtime вызовами.
                    // Для frontend-теста они
                    // не блокируют UI.
                    //
                    ping:
                        0,

                    using:
                        0,

                    is_dev_serv:
                        group.developer
                            ? 1
                            : 0
                });

                if (serverInfo.name ===
                    "Cluster SPB")
                {
                    result.defaultServerId =
                        id;
                }
            }
        }

        if (!result.defaultServerId &&
            result.list.length !==
                0)
        {
            result.defaultServerId =
                result.list[0].id;
        }
    }
    catch (error)
    {
        trace(
            "Server config read failed: " +
            error);
    }

    return result;
}


async function getLocales()
{
    try
    {
        const response =
            await fetch(
                LOCALES_CONFIG,
                {
                    cache:
                        "no-store"
                });

        return await response.json();
    }
    catch (error)
    {
        trace(
            "Locales read failed: " +
            error);

        return {};
    }
}


// --------------------------------------------------
// Original SOnline ExternalInterface
// --------------------------------------------------

window.getClientVersion =
    async function()
    {
        transmitAsync(
            "getClientVersion",
            {
                result:
                    await readVersion()
            });
    };


window.isInGame =
    function()
    {
        transmitAsync(
            "isInGame",
            {
                result:
                    0
            });
    };


window.getLocalesList =
    async function()
    {
        transmitAsync(
            "getLocalesList",
            await getLocales());
    };


window.getCurrentLocale =
    function()
    {
        transmitAsync(
            "getCurrentLocale",
            {
                locale:
                    currentLocale
            });
    };


window.setCurrentLocale =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        currentLocale =
            normalizeLocale(
                args.id);

        localStorage.setItem(
            "zone.locale",
            currentLocale);

        //
        // В оригинале applyLanguage
        // инициировал restart frontend/game.
        //
        setTimeout(
            () =>
            {
                location.reload();
            },
            0);
    };


window.getLocalizedResource =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        let paths =
            args.paths;

        if (!Array.isArray(
            paths))
        {
            paths =
                [];
        }

        postToHost(
            "localize",
            currentLocale,
            ...paths);
    };


window.quitGame =
    function()
    {
        postToHost(
            "quit");
    };


window.doRestartGame =
    function()
    {
        location.reload();
    };


window.openURL =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        postToHost(
            "open_url",
            args.url ||
            "");
    };


window.show_dummy =
    function()
    {
    };


window.hide_dummy =
    function()
    {
    };


window.play_sound =
    function()
    {
        //
        // Frontend sounds подключим
        // отдельно к оригинальным SO sfx.
        //
    };


// --------------------------------------------------
// Login
// --------------------------------------------------

window.ActionsWithLogin =
{
    getLogin:
        async function()
        {
            transmitAsync(
                "ActionsWithLogin.getLogin",
                {
                    login:
                        await readRememberedLogin()
                });
        },


    getServerList:
        async function()
        {
            transmitAsync(
                "ActionsWithLogin.getServerList",
                await buildServerList());
        },


    authenticateUser:
        function(rawArguments)
        {
            const args =
                unwrapArguments(
                    rawArguments);

            const remember =
                args.rememberMe === true ||
                args.rememberMe === 1 ||
                args.rememberMe === "1" ||
                args.rememberMe === "true";

            postToHost(
                "login",
                args.login ||
                    "",
                args.password ||
                    "",
                remember
                    ? "1"
                    : "0",
                args.serverID ??
                    "0");
        }
};


// --------------------------------------------------
// Settings API
//
// UI остаётся оригинальным.
// Здесь пока только transport placeholders,
// без рисования второго Settings UI.
// --------------------------------------------------

function emptyResponse(
    eventName)
{
    transmitAsync(
        eventName,
        {});
}


window.getSettingsRange =
    function()
    {
        emptyResponse(
            "getSettingsRange");
    };


window.getSettings =
    function()
    {
        emptyResponse(
            "getSettings");
    };


window.getDefaultOption =
    function()
    {
        emptyResponse(
            "getDefaultOption");
    };


window.getKeybindLocalizeTable =
    function()
    {
        emptyResponse(
            "getKeybindLocalizeTable");
    };


window.getNewKeybind =
    function()
    {
        emptyResponse(
            "getNewKeybind");
    };


window.setDefaultKeyBindings =
    function()
    {
    };


window.setSettings =
    function()
    {
    };


window.showOptionsMenu =
    function()
    {
    };


window.gui_reset_position =
    function()
    {
    };


// --------------------------------------------------
// Character / account API
//
// На текущем временном аккаунте персонажей нет.
// Поэтому отдаём реальное состояние:
// empty character list.
// --------------------------------------------------

window.allCharactersInfo =
    function()
    {
        transmitAsync(
            "allCharactersInfo",
            {
                list: [],
                currentId: 0
            });

        transmitAsync(
            "activateFirstCharWindow",
            {});
    };


window.selectChar =
    function()
    {
    };


window.creatingChar =
    function()
    {
    };


window.deleteCharacter =
    function()
    {
    };


window.restoreCharacter =
    function()
    {
    };


window.updatePremium =
    function()
    {
    };


window.checkAvatarName =
    function()
    {
    };


window.newCharView =
    function()
    {
    };


window.newFullCharView =
    function()
    {
    };


window.createChar =
    function()
    {
    };


window.cancelCreateChar =
    function()
    {
    };


window.showNews =
    function()
    {
        transmitAsync(
            "showNews",
            {
                text: []
            });
    };


window.reject_prem =
    function()
    {
    };


window.goToGame =
    function()
    {
        postToHost(
            "play");
    };


window.return_in_game =
    function()
    {
    };


// --------------------------------------------------
// C++ -> Flash
// --------------------------------------------------

window.ZoneFrontend =
{
    loginError:
        function(message)
        {
            //
            // Это точный event name,
            // который использовал
            // LoginScreenFlash.state_of_connect.
            //
            transmit(
                "ActionsWithLogin.authenticateUser",
                {
                    message:
                        message
                });
        },


    loginAccepted:
        function()
        {
            //
            // LoginScreenFlash.activate_account_window()
            // отправлял именно это событие.
            //
            transmit(
                "activateAccountWindow",
                {});
        },


    localizationResult:
        function(data)
        {
            transmit(
                "getLocalizedResource",
                data ||
                {});
        }
};


// --------------------------------------------------
// Startup screen
// --------------------------------------------------

function delay(milliseconds)
{
    return new Promise(
        resolve =>
            setTimeout(
                resolve,
                milliseconds));
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
            "Unable to load appStart.tga");
    }

    const bytes =
        new Uint8Array(
            await response.arrayBuffer());

    if (bytes.length <
        18)
    {
        throw new Error(
            "Invalid TGA header");
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

    if (colorMapType !==
        0)
    {
        throw new Error(
            "Color mapped TGA is unsupported");
    }

    if (imageType !== 2 &&
        imageType !== 3)
    {
        throw new Error(
            "Unsupported TGA image type");
    }

    if (bitsPerPixel !== 8 &&
        bitsPerPixel !== 24 &&
        bitsPerPixel !== 32)
    {
        throw new Error(
            "Unsupported TGA pixel format");
    }

    const bytesPerPixel =
        bitsPerPixel /
        8;

    let source =
        18 +
        idLength;

    const topOrigin =
        (descriptor &
            0x20) !==
        0;

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

            if (imageType ===
                3)
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
                bytesPerPixel ===
                    4
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


// --------------------------------------------------
// Real MainMenuGUI.swf
// --------------------------------------------------

async function startFlash()
{
    if (!window.RufflePlayer)
    {
        throw new Error(
            "RufflePlayer is unavailable");
    }

    const ruffle =
        window
            .RufflePlayer
            .newest();

    player =
        ruffle.createPlayer();

    player.id =
        "soMainMenu";

    const container =
        document.getElementById(
            "flash");

    container.appendChild(
        player);

    trace(
    "Loading original MainMenuGUI.swf: " +
    SWF_URL);

	await player
		.ruffle()
		.load({
			url:
				SWF_URL,

			base:
				SWF_BASE,

			allowScriptAccess:
				true,

			autoplay:
				"on",

			unmuteOverlay:
				"hidden",

			warnOnUnsupportedContent:
				true,

			logLevel:
				"warn",

			backgroundColor:
				"#000000",

			contextMenu:
				"off",

			preloader:
				false,

			splashScreen:
				false,

			letterbox:
				"on",

			scale:
				"showAll"
		});

	trace(
		"MainMenuGUI.swf load() completed");

    document
        .getElementById(
            "startup")
        .style
        .display =
            "none";

    container.style.display =
        "block";

    postToHost(
        "ready");
}


async function main()
{
    try
    {
        await Promise.all([
            drawStartupTga(),
            delay(
                STARTUP_DURATION)
        ]);

        await startFlash();
    }
    catch (error)
    {
        trace(
            "Frontend startup failed: " +
            (
                error &&
                error.stack
                    ? error.stack
                    : error
            ));

        console.error(
            error);
    }
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
	
// --------------------------------------------------
// ExternalInterface diagnostics
// --------------------------------------------------

function safeDebugValue(value)
{
    try
    {
        return JSON.stringify(value);
    }
    catch
    {
        return String(value);
    }
}


function traceGlobalFunction(name)
{
    const original =
        window[name];

    if (typeof original !==
        "function")
    {
        return;
    }

    window[name] =
        function(...args)
        {
            trace(
                "ExternalInterface -> " +
                name +
                " args=" +
                safeDebugValue(args));

            try
            {
                const result =
                    original.apply(
                        this,
                        args);

                return result;
            }
            catch (error)
            {
                trace(
                    "ExternalInterface ERROR -> " +
                    name +
                    ": " +
                    (
                        error &&
                        error.stack
                            ? error.stack
                            : error
                    ));

                throw error;
            }
        };
}

function traceObjectFunction(
    object,
    objectName,
    methodName)
{
    if (!object ||
        typeof object[methodName] !==
            "function")
    {
        return;
    }

    const original =
        object[methodName];

    object[methodName] =
        function(...args)
        {
            trace(
                "ExternalInterface -> " +
                objectName +
                "." +
                methodName +
                " args=" +
                safeDebugValue(args));

            try
            {
                return original.apply(
                    this,
                    args);
            }
            catch (error)
            {
                trace(
                    "ExternalInterface ERROR -> " +
                    objectName +
                    "." +
                    methodName +
                    ": " +
                    (
                        error &&
                        error.stack
                            ? error.stack
                            : error
                    ));

                throw error;
            }
        };
}

// Original top-level SO callbacks
[
    "getClientVersion",
    "isInGame",
    "getLocalizedResource",
    "quitGame",
    "doRestartGame",
    "openURL",
    "getLocalesList",
    "show_dummy",
    "hide_dummy",
    "play_sound",

    "getSettingsRange",
    "getSettings",
    "setDefaultKeyBindings",
    "setSettings",
    "showOptionsMenu",
    "getDefaultOption",
    "getKeybindLocalizeTable",
    "getCurrentLocale",
    "setCurrentLocale",
    "getNewKeybind",
    "gui_reset_position",

    "allCharactersInfo",
    "selectChar",
    "creatingChar",
    "deleteCharacter",
    "goToGame",
    "restoreCharacter",
    "updatePremium",
    "checkAvatarName",
    "newCharView",
    "newFullCharView",
    "createChar",
    "cancelCreateChar",
    "showNews",
    "reject_prem",
    "return_in_game"
].forEach(
    traceGlobalFunction);

// Login object callbacks
[
    "authenticateUser",
    "getLogin",
    "getServerList"
].forEach(
    method =>
        traceObjectFunction(
            window.ActionsWithLogin,
            "ActionsWithLogin",
            method));

main();