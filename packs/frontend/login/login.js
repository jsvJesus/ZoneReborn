"use strict";

(() =>
{
    let mountedRoot =
        null;


    const texts =
    {
        russian:
        {
            caption:
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

            invalid:
                "Неверный логин или пароль.",

            scene:
                "Не удалось загрузить сцену выбора персонажа."
        },

        english:
        {
            caption:
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

            invalid:
                "Invalid login or password.",

            scene:
                "Unable to load character selection scene."
        },

        chinese:
        {
            caption:
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

            invalid:
                "账号或密码错误。",

            scene:
                "无法加载角色选择场景。"
        }
    };


    function element(
        id)
    {
        return mountedRoot
            ? mountedRoot.querySelector(
                "#" + id)
            : null;
    }


    function text()
    {
        return texts[
            Frontend.getLocale()] ||
            texts.russian;
    }


    function updateButton()
    {
        const login =
            element(
                "loginName");

        const password =
            element(
                "loginPassword");

        const button =
            element(
                "loginButton");

        if (!login ||
            !password ||
            !button)
        {
            return;
        }

        button.disabled =
            login.value.trim().length === 0 ||
            password.value.length === 0;
    }


    function translateError(
        message)
    {
        switch (
            String(
                message ||
                ""))
        {
            case "Invalid login or password.":
                return text().invalid;

            case "Unable to initialize character selection scene.":
                return text().scene;

            default:
                return String(
                    message ||
                    "");
        }
    }


    function loginError(
        message)
    {
        const output =
            element(
                "loginError");

        if (output)
        {
            output.textContent =
                translateError(
                    message);
        }

        updateButton();
    }


    async function mount(
        root)
    {
        mountedRoot =
            root;

        const value =
            text();

        element(
            "loginCaption").
            textContent =
                value.caption;

        element(
            "loginName").
            placeholder =
                value.login;

        element(
            "loginPassword").
            placeholder =
                value.password;

        element(
            "loginRememberText").
            textContent =
                value.remember;

        element(
            "loginButton").
            textContent =
                value.enter;

        element(
            "loginRegister").
            textContent =
                value.register;

        element(
            "loginForgot").
            textContent =
                value.forgot;


        const savedLogin =
            await Frontend.
                readRememberedLogin();

        if (savedLogin)
        {
            element(
                "loginName").
                value =
                    savedLogin;

            element(
                "loginRemember").
                checked =
                    true;
        }


        element(
            "loginVersion").
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
            "loginName").
            addEventListener(
                "input",
                updateButton);

        element(
            "loginPassword").
            addEventListener(
                "input",
                updateButton);


        element(
            "loginPassword").
            addEventListener(
                "keydown",
                event =>
                {
                    if (event.key ===
                        "Enter")
                    {
                        element(
                            "loginButton").
                            click();
                    }
                });


        element(
            "loginButton").
            addEventListener(
                "click",
                () =>
                {
                    const login =
                        element(
                            "loginName").
                            value.
                            trim();

                    const password =
                        element(
                            "loginPassword").
                            value;

                    const remember =
                        element(
                            "loginRemember").
                            checked;

                    element(
                        "loginButton").
                        disabled =
                            true;

                    element(
                        "loginError").
                        textContent =
                            "";

                    Frontend.login(
                        login,
                        password,
                        remember);
                });


        element(
            "loginRegister").
            addEventListener(
                "click",
                () =>
                    Frontend.openUrl(
                        "register"));


        element(
            "loginForgot").
            addEventListener(
                "click",
                () =>
                    Frontend.openUrl(
                        "forgot-password"));


        element(
            "loginClose").
            addEventListener(
                "click",
                () =>
                    Frontend.quit());


        updateButton();
    }


    FrontendScreens.register(
        "login",
        {
            template:
                "./login/login.html",

            mount:
                mount,

            loginError:
                loginError
        });
})();