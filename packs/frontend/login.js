"use strict";

(() =>
{
    const LAST_SERVER_KEY =
        "frontend.last.server";


    const texts =
    {
        russian:
        {
            authorize:
                "АВТОРИЗУЙТЕСЬ",

            login:
                "Логин",

            password:
                "Пароль",

            remember:
                "Запомнить меня",

            enter:
                "ВОЙТИ",

            register:
                "Зарегистрироваться",

            forgot:
                "Не помню пароль",

            servers:
                "ВЫБЕРИТЕ СЕРВЕР",

            connect:
                "ПОДКЛЮЧИТЬСЯ",

            back:
                "НАЗАД",

            online:
                "Online",

            offline:
                "Offline",

            emptyServers:
                "Нет доступных серверов.",

            invalidLogin:
                "Неверный логин или пароль.",

            loginRequired:
                "Сначала необходимо авторизоваться.",

            serverRequired:
                "Выберите сервер.",

            sceneError:
                "Не удалось загрузить сцену выбора персонажа."
        },

        english:
        {
            authorize:
                "AUTHORIZE",

            login:
                "Login",

            password:
                "Password",

            remember:
                "Remember me",

            enter:
                "LOG IN",

            register:
                "Register",

            forgot:
                "Forgot password",

            servers:
                "SELECT SERVER",

            connect:
                "CONNECT",

            back:
                "BACK",

            online:
                "Online",

            offline:
                "Offline",

            emptyServers:
                "No servers available.",

            invalidLogin:
                "Invalid login or password.",

            loginRequired:
                "Authentication is required.",

            serverRequired:
                "Select a server.",

            sceneError:
                "Unable to load character selection scene."
        },

        chinese:
        {
            authorize:
                "登录",

            login:
                "账号",

            password:
                "密码",

            remember:
                "记住我",

            enter:
                "登录",

            register:
                "注册",

            forgot:
                "忘记密码",

            servers:
                "选择服务器",

            connect:
                "连接",

            back:
                "返回",

            online:
                "Online",

            offline:
                "Offline",

            emptyServers:
                "没有可用服务器。",

            invalidLogin:
                "账号或密码错误。",

            loginRequired:
                "请先登录。",

            serverRequired:
                "请选择服务器。",

            sceneError:
                "无法加载角色选择场景。"
        }
    };


    const state =
    {
        busy:
            false,

        selectedServer:
            "",

        servers:
            [],

        login:
            ""
    };


    function element(
        id)
    {
        return document.getElementById(
            id);
    }


    function bridge()
    {
        return window.FrontendBridge;
    }


    function currentLocale()
    {
        const value =
            bridge() &&
            bridge().getLocale
                ? bridge().getLocale()
                : "russian";

        if (texts[value])
        {
            return value;
        }

        return "russian";
    }


    function text()
    {
        return texts[
            currentLocale()];
    }


    function applyLocalization()
    {
        const value =
            text();

        element(
            "nativeLoginCaption").
            textContent =
                value.authorize;

        element(
            "nativeLoginName").
            placeholder =
                value.login;

        element(
            "nativeLoginPassword").
            placeholder =
                value.password;

        element(
            "nativeRememberText").
            textContent =
                value.remember;

        element(
            "nativeLoginButton").
            textContent =
                value.enter;

        element(
            "nativeRegisterButton").
            textContent =
                value.register;

        element(
            "nativeForgotButton").
            textContent =
                value.forgot;

        element(
            "nativeServerCaption").
            textContent =
                value.servers;

        element(
            "nativeServerButton").
            textContent =
                value.connect;

        element(
            "nativeServerBack").
            textContent =
                value.back;


        document
            .querySelectorAll(
                "[data-login-locale]")
            .forEach(
                button =>
                {
                    button.classList.toggle(
                        "selected",
                        button.dataset.loginLocale ===
                            currentLocale());
                });
    }


    function translateError(
        message)
    {
        const value =
            text();

        switch (
            String(
                message ||
                ""))
        {
            case "Invalid login or password.":
                return value.invalidLogin;

            case "Authentication required.":
                return value.loginRequired;

            case "Server id is empty.":
                return value.serverRequired;

            case "Unable to initialize character selection scene.":
                return value.sceneError;

            default:
                return String(
                    message ||
                    "");
        }
    }


    function setAuthError(
        message)
    {
        element(
            "nativeLoginError").
            textContent =
                translateError(
                    message);
    }


    function setServerError(
        message)
    {
        element(
            "nativeServerError").
            textContent =
                translateError(
                    message);
    }


    function setBusy(
        value)
    {
        state.busy =
            !!value;

        updateAuthButton();
        updateServerButton();
    }


    function updateAuthButton()
    {
        const login =
            element(
                "nativeLoginName").
                value.trim();

        const password =
            element(
                "nativeLoginPassword").
                value;

        element(
            "nativeLoginButton").
            disabled =
                state.busy ||
                login.length === 0 ||
                password.length === 0 ||
                login.length > 40 ||
                password.length > 40;
    }


    function updateServerButton()
    {
        element(
            "nativeServerButton").
            disabled =
                state.busy ||
                state.selectedServer.length === 0;
    }


    function showAuthorize()
    {
        element(
            "nativeAuthorizeCard").
            hidden =
                false;

        element(
            "nativeServerCard").
            hidden =
                true;

        setAuthError("");
        setServerError("");

        setBusy(
            false);

        setTimeout(
            () =>
            {
                const login =
                    element(
                        "nativeLoginName");

                const password =
                    element(
                        "nativeLoginPassword");

                if (login.value.length)
                {
                    password.focus();
                }
                else
                {
                    login.focus();
                }
            },
            0);
    }


    function showServers()
    {
        element(
            "nativeAuthorizeCard").
            hidden =
                true;

        element(
            "nativeServerCard").
            hidden =
                false;

        setAuthError("");
        setServerError("");

        setBusy(
            false);
    }


    function selectServer(
        uid)
    {
        state.selectedServer =
            String(
                uid ||
                "");

        document
            .querySelectorAll(
                ".native-server-row")
            .forEach(
                row =>
                {
                    row.classList.toggle(
                        "selected",
                        row.dataset.serverUid ===
                            state.selectedServer);
                });

        updateServerButton();
    }


    function renderServers()
    {
        const list =
            element(
                "nativeServerList");

        list.replaceChildren();

        for (const server of
            state.servers)
        {
            const uid =
                String(
                    server.uid ||
                    server.id ||
                    "");

            const row =
                document.createElement(
                    "button");

            row.type =
                "button";

            row.className =
                "native-server-row";

            row.dataset.serverUid =
                uid;


            const name =
                document.createElement(
                    "span");

            name.className =
                "native-server-name";

            name.textContent =
                String(
                    server.label ||
                    uid);


            const status =
                document.createElement(
                    "span");

            const online =
                server.online !==
                    false;

            status.className =
                "native-server-status " +
                (
                    online
                        ? "online"
                        : "offline"
                );


            const indicator =
                document.createElement(
                    "span");

            indicator.className =
                "native-server-indicator " +
                (
                    online
                        ? "online"
                        : "offline"
                );


            const statusText =
                document.createElement(
                    "span");

            statusText.textContent =
                online
                    ? text().online
                    : text().offline;


            status.appendChild(
                indicator);

            status.appendChild(
                statusText);

            row.appendChild(
                name);

            row.appendChild(
                status);


            row.addEventListener(
                "click",
                () =>
                {
                    selectServer(
                        uid);
                });


            list.appendChild(
                row);
        }


        const lastServer =
            localStorage.getItem(
                LAST_SERVER_KEY) ||
            "";

        if (lastServer &&
            state.servers.some(
                server =>
                    String(
                        server.uid ||
                        server.id ||
                        "") ===
                    lastServer))
        {
            selectServer(
                lastServer);
        }
        else
        {
            selectServer(
                "");
        }


        if (state.servers.length ===
            0)
        {
            setServerError(
                text().
                    emptyServers);
        }
    }


    async function submitLogin()
    {
        updateAuthButton();

        if (element(
                "nativeLoginButton").
                disabled)
        {
            return;
        }

        const login =
            element(
                "nativeLoginName").
                value.trim();

        const password =
            element(
                "nativeLoginPassword").
                value;

        const remember =
            element(
                "nativeRemember").
                checked;


        state.login =
            login;

        setAuthError("");

        setBusy(
            true);


        bridge().
            postLogin(
                login,
                password,
                remember);
    }


    async function authenticated()
    {
        setBusy(
            false);

        const data =
            await bridge().
                buildServerList();

        state.servers =
            Array.isArray(
                data &&
                data.list)
                    ? data.list
                    : [];

        renderServers();

        showServers();
    }


    function submitServer()
    {
        updateServerButton();

        if (element(
                "nativeServerButton").
                disabled)
        {
            return;
        }

        setServerError("");

        setBusy(
            true);

        bridge().
            selectServer(
                state.selectedServer);
    }


    function serverAccepted()
    {
        setBusy(
            false);

        if (state.selectedServer)
        {
            localStorage.setItem(
                LAST_SERVER_KEY,
                state.selectedServer);
        }

        hide();
    }


    function backToAuthorize()
    {
        bridge().
            logout();

        state.selectedServer =
            "";

        state.servers =
            [];

        showAuthorize();
    }


    async function show()
    {
        const root =
            element(
                "nativeLogin");

        root.hidden =
            false;


        applyLocalization();


        const savedLogin =
            await bridge().
                readRememberedLogin();

        if (savedLogin)
        {
            element(
                "nativeLoginName").
                value =
                    savedLogin;

            element(
                "nativeRemember").
                checked =
                    true;
        }


        const version =
            await bridge().
                readVersion();

        element(
            "nativeLoginVersion").
            textContent =
                version;


        showAuthorize();

        updateAuthButton();
    }


    function hide()
    {
        element(
            "nativeLogin").
            hidden =
                true;
    }


    function initialize()
    {
        element(
            "nativeLoginName").
            addEventListener(
                "input",
                updateAuthButton);

        element(
            "nativeLoginPassword").
            addEventListener(
                "input",
                updateAuthButton);


        element(
            "nativeLoginName").
            addEventListener(
                "keydown",
                event =>
                {
                    if (event.key ===
                        "Enter")
                    {
                        element(
                            "nativeLoginPassword").
                            focus();
                    }
                });


        element(
            "nativeLoginPassword").
            addEventListener(
                "keydown",
                event =>
                {
                    if (event.key ===
                        "Enter")
                    {
                        submitLogin();
                    }
                });


        element(
            "nativeLoginButton").
            addEventListener(
                "click",
                submitLogin);


        element(
            "nativeServerButton").
            addEventListener(
                "click",
                submitServer);


        element(
            "nativeServerBack").
            addEventListener(
                "click",
                backToAuthorize);


        element(
            "nativeRegisterButton").
            addEventListener(
                "click",
                () =>
                {
                    bridge().
                        openUrl(
                            "register");
                });


        element(
            "nativeForgotButton").
            addEventListener(
                "click",
                () =>
                {
                    bridge().
                        openUrl(
                            "forgot-password");
                });


        element(
            "nativeLoginClose").
            addEventListener(
                "click",
                () =>
                {
                    bridge().
                        quit();
                });


        document
            .querySelectorAll(
                "[data-login-locale]")
            .forEach(
                button =>
                {
                    button.addEventListener(
                        "click",
                        () =>
                        {
                            window.setCurrentLocale(
                                {
                                    id:
                                        button.dataset.loginLocale
                                });
                        });
                });


        updateAuthButton();
        updateServerButton();
    }


    window.NativeLogin =
    {
        show:
            show,

        hide:
            hide,

        authenticated:
            authenticated,

        loginError:
            function(
                message)
            {
                setBusy(
                    false);

                setAuthError(
                    message);
            },

        serverError:
            function(
                message)
            {
                setBusy(
                    false);

                setServerError(
                    message);
            },

        serverAccepted:
            serverAccepted
    };


    initialize();
})();