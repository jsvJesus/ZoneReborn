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
    "/packs/res/soGUI/maps/loadingScreen/appStart3.tga";

const SERVER_CONFIG =
    ROOT +
    "/packs/res/scripts/client/data/servers_config.json";

const LOCALES_CONFIG =
    ROOT +
    "/packs/res/local/localizations.json";
	
const CHAR_MAKER_CONFIG =
    ROOT +
    "/packs/res/scripts/client/data/charMakerCfg.json";

const TEST_CHARACTERS_KEY =
    "zone.test.characters";

const TEST_CURRENT_CHARACTER_KEY =
    "zone.test.currentCharacter";

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
	
let lastValidatedNickname =
    "";

let pendingCharacterAppearance =
    {};

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
    if (!player)
    {
        return;
    }

    const message = {
        event_name:
            eventName,

        data:
            data ??
            {}
    };

    trace(
        "Host -> Flash event=" +
        eventName +
        " data=" +
        safeDebugValue(
            message.data));

    try
    {
        if (typeof player.externalInterfaceTransmit ===
            "function")
        {
            player.externalInterfaceTransmit(
                message);

            return;
        }

        player
            .ruffle()
            .callExternalInterface(
                "externalInterfaceTransmit",
                message);
    }
    catch (error)
    {
        trace(
            "externalInterfaceTransmit ERROR: " +
            (
                error &&
                error.stack
                    ? error.stack
                    : error
            ));
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
        100);
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
                    "Cluster EU")
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


function getDummyPartId(
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


window.show_dummy =
    function()
    {
        trace(
            "show_dummy");

        postToHost(
            "dummy_show");

        sendCurrentCharacterAppearance();
    };


window.hide_dummy =
    function()
    {
        trace(
            "hide_dummy");

        postToHost(
            "dummy_hide");
    };
	
	
window.rotate_dummy =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        let dx =
            0;

        let dy =
            0;

        if (Array.isArray(
                args))
        {
            dx =
                Number(
                    args[0] ||
                    0);

            dy =
                Number(
                    args[1] ||
                    0);
        }

        postToHost(
            "dummy_rotate",
            dx,
            dy);
    };


window.play_sound =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        let soundName =
            "";

        if (typeof args ===
            "string")
        {
            soundName =
                args;
        }
        else if (Array.isArray(
                     args))
        {
            soundName =
                String(
                    args[0] ||
                    "");
        }
        else if (args &&
                 typeof args ===
                     "object")
        {
            soundName =
                String(
                    args.name ||
                    args.sound ||
                    "");
        }

        if (!soundName)
        {
            return;
        }

        trace(
            "play_sound: " +
            soundName);

        postToHost(
            "ui_sound",
            soundName);
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
        postToHost(
            "keybind_localize");
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


function loadTestCharacters()
{
    try
    {
        const raw =
            localStorage.getItem(
                TEST_CHARACTERS_KEY);

        if (!raw)
        {
            return [];
        }

        const result =
            JSON.parse(raw);

        return Array.isArray(result)
            ? result
            : [];
    }
    catch
    {
        return [];
    }
}


function saveTestCharacters(
    characters)
{
    localStorage.setItem(
        TEST_CHARACTERS_KEY,
        JSON.stringify(
            characters));
}


function getCurrentCharacterIndex(
    characters)
{
    if (!characters.length)
    {
        return -1;
    }

    const saved =
        Number.parseInt(
            localStorage.getItem(
                TEST_CURRENT_CHARACTER_KEY) ||
            "0",
            10);

    if (!Number.isFinite(saved) ||
        saved < 0 ||
        saved >= characters.length)
    {
        return 0;
    }

    return saved;
}

function getCharacterAppearanceEntries(
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

    const entries =
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

        entries.push(
            {
                choiceGroup:
                    choiceGroup,

                var:
                    value
            });
    }

    return entries;
}

function buildStoredCharacterAppearance()
{
    const random =
        [];

    for (const [
             choiceGroup,
             value
         ] of Object.entries(
             pendingCharacterAppearance))
    {
        if (!choiceGroup ||
            !value ||
            typeof value !==
                "object")
        {
            continue;
        }

        random.push(
            {
                choiceGroup:
                    choiceGroup,

                var:
                    value
            });
    }

    return {
        random:
            random
    };
}

function sendCurrentCharacterAppearance()
{
    const characters =
        loadTestCharacters();

    const currentId =
        getCurrentCharacterIndex(
            characters);

    if (currentId < 0 ||
        currentId >=
            characters.length)
    {
        return;
    }

    const character =
        characters[
            currentId];

    if (!character)
    {
        return;
    }

    const entries =
        getCharacterAppearanceEntries(
            character.appearance);

    const fields =
        [];

    for (const entry of entries)
    {
        if (!entry ||
            !entry.choiceGroup ||
            !entry.var)
        {
            continue;
        }

        const partId =
            getDummyPartId(
                entry.choiceGroup,
                entry.var);

        if (!Number.isFinite(
                partId) ||
            partId <= 0)
        {
            continue;
        }

        fields.push(
            entry.choiceGroup);

        fields.push(
            partId);
    }

    if (fields.length === 0)
    {
        trace(
            "Character appearance is empty for id=" +
            currentId);

        return;
    }

    trace(
        "Sending CharacterDummy appearance: id=" +
        currentId +
        ", parts=" +
        fields.length / 2);

    postToHost(
        "dummy_full",
        ...fields);
}


function validateCharacterName(
    nickname)
{
    if (nickname.length < 3 ||
        nickname.length > 32)
    {
        return false;
    }

    //
    // Latin + Cyrillic + numbers + _
    //
    if (!/^[A-Za-zА-Яа-яЁёІіЇїЄєҐґ0-9_]+$/u.test(
            nickname))
    {
        return false;
    }

    //
    // Оригинальное описание разрешает
    // один символ "_".
    //
    const underscores =
        nickname.match(/_/g);

    if (underscores &&
        underscores.length > 1)
    {
        return false;
    }

    return true;
}


async function loadCharacterMakerConfig()
{
    const response =
        await fetch(
            CHAR_MAKER_CONFIG,
            {
                cache:
                    "no-store"
            });

    if (!response.ok)
    {
        throw new Error(
            "Unable to load charMakerCfg.json");
    }

    return await response.json();
}

// --------------------------------------------------
// Character / account API
// --------------------------------------------------

window.allCharactersInfo =
    function()
    {
        const characters =
            loadTestCharacters();

        const currentId =
            getCurrentCharacterIndex(
                characters);

        transmitAsync(
            "allCharactersInfo",
            {
                list:
                    characters,

                currentId:
                    currentId
            });

        //
        // Оригинал открывает окно первого
        // персонажа ТОЛЬКО если список пустой.
        //
        if (characters.length ===
            0)
        {
            setTimeout(
                () =>
                {
                    transmit(
                        "activateFirstCharWindow",
                        {});
                },
                180);
        }
    };


window.selectChar =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        const id =
            Number(
                args.id);

        const characters =
            loadTestCharacters();

        if (Number.isInteger(id) &&
            id >= 0 &&
            id < characters.length)
        {
            localStorage.setItem(
                TEST_CURRENT_CHARACTER_KEY,
                String(id));

            sendCurrentCharacterAppearance();
        }
    };


window.checkAvatarName =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        const nickname =
            String(
                args.nick ||
                "")
                .trim();

        const characters =
            loadTestCharacters();

        const syntaxValid =
            validateCharacterName(
                nickname);

        const alreadyExists =
            characters.some(
                character =>
                    String(
                        character.name ||
                        "")
                        .toLowerCase() ===
                    nickname.toLowerCase());

        const valid =
            syntaxValid &&
            !alreadyExists;

        if (valid)
        {
            lastValidatedNickname =
                nickname;
        }

        let message =
            "";

        if (!syntaxValid)
        {
            message =
                "Недопустимое имя персонажа";
        }
        else if (alreadyExists)
        {
            message =
                "Такое имя уже занято";
        }

        transmitAsync(
            "checkAvatarName",
            {
                nick:
                    nickname,

                message:
                    message,

                result:
                    valid
                        ? 1
                        : 0
            });
    };


window.creatingChar =
    async function()
    {
        try
        {
            const config =
                await loadCharacterMakerConfig();

            transmitAsync(
                "creatingChar",
                config);
        }
        catch (error)
        {
            trace(
                "Character maker config failed: " +
                error);

            transmitAsync(
                "creatingChar",
                {});
        }
    };


window.newCharView =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        if (!args ||
            typeof args !==
                "object" ||
            !args.choiceGroup)
        {
            return;
        }

        pendingCharacterAppearance[
            args.choiceGroup] =
                args.var;

        const partId =
            getDummyPartId(
                args.choiceGroup,
                args.var);

        trace(
            "newCharView: " +
            args.choiceGroup +
            " -> " +
            partId);

        postToHost(
            "dummy_part",
            args.choiceGroup,
            partId);
    };


window.newFullCharView =
    function(rawArguments)
    {
        const args =
            unwrapArguments(
                rawArguments);

        if (!Array.isArray(
                args))
        {
            return;
        }

        const fields =
            [];

        for (const entry of args)
        {
            if (!entry ||
                !entry.choiceGroup)
            {
                continue;
            }

            pendingCharacterAppearance[
                entry.choiceGroup] =
                    entry.var;

            const partId =
                getDummyPartId(
                    entry.choiceGroup,
                    entry.var);

            fields.push(
                entry.choiceGroup);

            fields.push(
                partId);
        }

        trace(
            "newFullCharView parts=" +
            fields.length / 2);

        postToHost(
            "dummy_full",
            ...fields);
    };


window.createChar =
    function()
    {
        const nickname =
            String(
                lastValidatedNickname ||
                "")
                .trim();

        if (!validateCharacterName(
                nickname))
        {
            transmitAsync(
                "createChar",
                {
                    errcode:
                        1,

                    msg:
                        "Некорректное имя персонажа",

                    success:
                        0
                });

            return;
        }

        const characters =
            loadTestCharacters();

        const exists =
            characters.some(
                character =>
                    String(
                        character.name ||
                        "")
                        .toLowerCase() ===
                    nickname.toLowerCase());

        if (exists)
        {
            transmitAsync(
                "createChar",
                {
                    errcode:
                        2,

                    msg:
                        "Персонаж с таким именем уже существует",

                    success:
                        0
                });

            return;
        }

        const newCharacter = {
            id:
                characters.length,

            name:
                nickname,

            maxspeed:
                5.0,

            maxhp:
                100,

            hp_regen:
                1.0,

            maxstamina:
                100,

            stamina_regen:
                1.0,

            maxweight:
                50,

            isTutorialPassed:
                0,

            deletion_remaining_time:
                -1,

            goldCredit:
                0,

            appearance:
				buildStoredCharacterAppearance()
        };

        characters.push(
            newCharacter);

        saveTestCharacters(
            characters);

        localStorage.setItem(
            TEST_CURRENT_CHARACTER_KEY,
            String(
                characters.length -
                1));

        pendingCharacterAppearance =
            {};

        transmitAsync(
            "createChar",
            {
                errcode:
                    0,

                msg:
                    "",

                success:
                    1
            });
    };


window.cancelCreateChar =
    function()
    {
        pendingCharacterAppearance =
            {};
    };


window.deleteCharacter =
    function()
    {
        const characters =
            loadTestCharacters();

        const index =
            getCurrentCharacterIndex(
                characters);

        if (index < 0)
        {
            return;
        }

        characters.splice(
            index,
            1);

        for (let i = 0;
             i < characters.length;
             ++i)
        {
            characters[i].id =
                i;
        }

        saveTestCharacters(
            characters);

        localStorage.setItem(
            TEST_CURRENT_CHARACTER_KEY,
            "0");
    };


window.restoreCharacter =
    function()
    {
    };


window.updatePremium =
    function()
    {
    };


// --------------------------------------------------
// C++ -> Flash
// --------------------------------------------------

window.ZoneFrontend =
{
    keybindLocalizeResult:
        function(data)
        {
            transmit(
                "getKeybindLocalizeTable",
                data ||
                {});
        },


    loginError:
        function(message)
        {
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
				null,
				
			wmode:
				"transparent",

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
		
	trace(
		"After load: externalInterfaceTransmit typeof=" +
			typeof player.externalInterfaceTransmit);

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
	"rotate_dummy",
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
