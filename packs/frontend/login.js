"use strict";

(() =>
{
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

            invalidLogin:
                "Неверный логин или пароль.",

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

            invalidLogin:
                "Invalid login or password.",

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

            invalidLogin:
                "账号或密码错误。",

            sceneError:
                "无法加载角色选择场景。"
        }
    };


    const state =
    {
        busy:
            false
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


    function setBusy(
        value)
    {
        state.busy =
            !!value;

        updateAuthButton();
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


    function submitLogin()
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

        setAuthError("");

        setBusy(
            true);

        bridge().
            postLogin(
                login,
                password,
                remember);
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

        setAuthError("");

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


    function hide()
    {
        element(
            "nativeLogin").
            hidden =
                true;
    }


    function loginComplete()
    {
        setBusy(
            false);

        hide();
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
    }


    window.NativeLogin =
    {
        show:
            show,

        hide:
            hide,

        loginComplete:
            loginComplete,

        loginError:
            function(
                message)
            {
                setBusy(
                    false);

                setAuthError(
                    message);
            }
    };


    initialize();
})();