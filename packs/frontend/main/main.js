"use strict";

(() =>
{
    let mountedRoot =
        null;

    let rotating =
        false;

    let rotatingPointer =
        -1;

    let lastMouseX =
        0;

    let lastMouseY =
        0;


    const translations =
    {
        russian:
        {
            mainTitle:
                "ГЛАВНОЕ МЕНЮ",

            startGame:
                "НАЧАТЬ ИГРУ",

            myCharacter:
                "МОЙ ПЕРСОНАЖ",

			readyCharacter:
				"ГОТОВ К ВЫХОДУ В ЗОНУ",

            settings:
                "НАСТРОЙКИ",

            partner:
                "ПАРТНЕРСКАЯ ПРОГРАММА",

            promo:
                "ВВЕСТИ ПРОМОКОД",

            support:
                "ТЕХПОДДЕРЖКА",

            news:
                "НОВОСТИ",

            license:
                "ЛИЦЕНЗИОННОЕ СОГЛАШЕНИЕ",

            exit:
                "ВЫХОД",

            store:
                "МАГАЗИН",

            storage:
                "СКЛАД",

            appearanceTitle:
                "МОДИФИКАЦИЯ ВНЕШНОСТИ",

            appearanceDescription:
                "Здесь доступны любые формы изменения внешности, если она Вам надоела.",

            equipmentTitle:
                "МОДИФИКАЦИЯ ЭКИПИРОВКИ",

            equipmentDescription:
                "Изменяйте внешний вид своей одежды и собирайте уникальный комплект.",

            premiumTitle:
                "ПРЕМИАЛЬНАЯ ПОДПИСКА",

            premiumDescription:
                "Подключите, чтобы получать увеличенный опыт и другие преимущества.",
				
			storeTitle:
				"МАГАЗИН ПРЕДМЕТОВ",
				
			storeDescription:
				"Магазин, где продаются предметы и различные предметы кастомизации.",
				
			storageTitle:
				"ХРАНИЛИЩЕ",
				
			storageDescription:
				"Все приобретенные вами товары доставлены на склад.",

            deleteCharacter:
                "Удалить персонажа",

            copyId:
                "Скопировать ID",

            copiedId:
                "ID скопирован"
        },


        english:
        {
            mainTitle:
                "MAIN MENU",

            startGame:
                "START GAME",

            myCharacter:
                "MY CHARACTER",

			readyCharacter:
				"READY TO ENTER THE ZONE",

            settings:
                "SETTINGS",

            partner:
                "PARTNER PROGRAM",

            promo:
                "ENTER PROMO CODE",

            support:
                "SUPPORT",

            news:
                "NEWS",

            license:
                "LICENSE AGREEMENT",

            exit:
                "EXIT",

            store:
                "STORE",

            storage:
                "STORAGE",

            appearanceTitle:
                "APPEARANCE MODIFICATION",

            appearanceDescription:
                "Change your character appearance whenever you want.",

            equipmentTitle:
                "EQUIPMENT MODIFICATION",

            equipmentDescription:
                "Change the appearance of your equipment and create a unique set.",

            premiumTitle:
                "PREMIUM SUBSCRIPTION",

            premiumDescription:
                "Receive increased experience and other premium benefits.",
				
			storeTitle:
				"MARKET",
				
			storeDescription:
				"A shop where items and various customization items are sold.",
				
			storageTitle:
				"STORAGE",
				
			storageDescription:
				"All the items you purchased have been delivered to the warehouse.",

            deleteCharacter:
                "Delete character",

            copyId:
                "Copy ID",

            copiedId:
                "ID copied"
        },


        chinese:
        {
            mainTitle:
                "主菜单",

            startGame:
                "开始游戏",

            myCharacter:
                "我的角色",

			readyCharacter:
				"准备进入区域",

            settings:
                "设置",

            partner:
                "合作伙伴计划",

            promo:
                "输入促销代码",

            support:
                "技术支持",

            news:
                "新闻",

            license:
                "许可协议",

            exit:
                "退出",

            store:
                "商店",

            storage:
                "仓库",

            appearanceTitle:
                "外观修改",

            appearanceDescription:
                "您可以随时修改角色的外观。",

            equipmentTitle:
                "装备外观修改",

            equipmentDescription:
                "修改装备外观并创建独特的套装。",

            premiumTitle:
                "高级订阅",

            premiumDescription:
                "获得额外经验以及其他高级特权。",
				
			storeTitle:
				"店铺",
				
			storeDescription:
				"出售各类物品及各种定制化物品的商店。",
				
			storageTitle:
				"贮存",
				
			storageDescription:
				"您购买的所有商品均已送至仓库。",

            deleteCharacter:
                "删除角色",

            copyId:
                "复制 ID",

            copiedId:
                "ID 已复制"
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


    function translation(
        key)
    {
        const locale =
            Frontend.getLocale();

        const table =
            translations[locale] ||
            translations.russian;

        return table[key] ||
            translations.russian[key] ||
            key;
    }


    function applyTranslations()
    {
        if (!mountedRoot)
        {
            return;
        }

        mountedRoot
            .querySelectorAll(
                "[data-i18n]")
            .forEach(
                node =>
                {
                    const key =
                        node.dataset.i18n;

                    node.textContent =
                        translation(
                            key);
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


        const accountId =
            element(
                "mainAccountId");

        if (accountId)
        {
            accountId.title =
                translation(
                    "copyId");
        }
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


    function renderAccount()
    {
        const name =
            element(
                "mainAccountName");

        const accountId =
            element(
                "mainAccountId");

        const softBalance =
            element(
                "mainSoftBalanceValue");

        const premiumBalance =
            element(
                "mainPremiumBalanceValue");


        name.textContent =
            Frontend.state.accountLogin ||
            "PLAYER";


        const id =
            Frontend.state.accountId ?? "";

        accountId.dataset.value =
            String(
                id);

        accountId.textContent =
            id === "" ||
            id === null ||
            id === undefined
                ? "ID —"
                : "ID " +
                  String(
                      id);


        softBalance.textContent =
            formatBalance(
                Frontend.state.softCurrency ??
                0);

        premiumBalance.textContent =
            formatBalance(
                Frontend.state.premiumCurrency ??
                0);
    }


    async function copyAccountId()
    {
        const button =
            element(
                "mainAccountId");

        if (!button)
        {
            return;
        }

        const value =
            String(
                button.dataset.value ||
                "");

        if (!value)
        {
            return;
        }


        try
        {
            await navigator.clipboard.writeText(
                value);
        }
        catch
        {
            const textarea =
                document.createElement(
                    "textarea");

            textarea.value =
                value;

            textarea.style.position =
                "fixed";

            textarea.style.opacity =
                "0";

            document.body.appendChild(
                textarea);

            textarea.select();

            document.execCommand(
                "copy");

            textarea.remove();
        }


        button.classList.add(
            "copied");

        button.title =
            translation(
                "copiedId");

        window.setTimeout(
            () =>
            {
                if (!button.isConnected)
                {
                    return;
                }

                button.classList.remove(
                    "copied");

                button.title =
                    translation(
                        "copyId");
            },
            900);
    }


    function applyCurrentCharacter()
    {
        const character =
            CharacterStore.current();

        if (!character)
        {
            Frontend.hideCharacter();

            return;
        }

        Frontend.showCharacter();


        const fields =
            CharacterStore.appearanceFields(
                character);

        if (!fields.length)
        {
            return;
        }

        Frontend.setCharacterFull(
            fields);
    }


    function renderCharacter()
	{
		const container =
			element(
				"mainCharacterList");

		const play =
			element(
				"mainPlay");

		container.replaceChildren();

		const character =
			CharacterStore.current();

		if (!character)
		{
			play.disabled =
				true;

			Frontend.hideCharacter();

			return;
		}

		play.disabled =
			false;

		const row =
			document.createElement(
				"div");

		row.className =
			"main-character";

		const avatar =
			document.createElement(
				"div");

		avatar.className =
			"main-character-avatar";

		const avatarCanvas =
			document.createElement(
				"canvas");

		avatarCanvas.className =
			"main-tga";

		avatarCanvas.dataset.tga =
			"/packs/res/soGUI/maps/MainMenu/Card/card_icon_character.tga";

		avatar.appendChild(
			avatarCanvas);

		const info =
			document.createElement(
				"div");

		info.className =
			"main-character-info";

		const active =
			document.createElement(
				"div");

		active.className =
			"main-character-active";

		const name =
			document.createElement(
				"div");

		name.className =
			"main-character-name";

		name.textContent =
			String(
				character.name ||
				"Character");

		const status =
			document.createElement(
				"div");

		status.className =
			"main-character-status";

		const statusDot =
			document.createElement(
				"span");

		statusDot.className =
			"main-character-status-dot";

		const statusText =
			document.createElement(
				"span");

		statusText.textContent =
			translation(
				"readyCharacter");

		status.appendChild(
			statusDot);

		status.appendChild(
			statusText);

		info.appendChild(
			active);

		info.appendChild(
			name);

		info.appendChild(
			status);

		const remove =
			document.createElement(
				"button");

		remove.type =
			"button";

		remove.className =
			"main-character-delete";

		remove.title =
			translation(
				"deleteCharacter");

		const icon =
			document.createElement(
				"img");

		icon.src =
			"/packs/res/soGUI/frame/storehouse/x_new.png";

		icon.alt =
			"";

		remove.appendChild(
			icon);

		remove.addEventListener(
			"click",
			event =>
			{
				event.stopPropagation();

				if (!CharacterStore.removeCurrent())
				{
					return;
				}

				Frontend.hideCharacter();

				renderCharacter();

				Frontend.trace(
					"Current character deleted.");
			});

		row.appendChild(
			avatar);

		row.appendChild(
			info);

		row.appendChild(
			remove);

		container.appendChild(
			row);

		loadTga(
			avatarCanvas);
	}


    function bindCharacterRotation()
    {
        const area =
            element(
                "characterRotateArea");

        if (!area)
        {
            return;
        }


        area.addEventListener(
            "contextmenu",
            event =>
            {
                event.preventDefault();
            });


        area.addEventListener(
            "pointerdown",
            event =>
            {
                if (event.button !== 2)
                {
                    return;
                }

                event.preventDefault();


                rotating =
                    true;

                rotatingPointer =
                    event.pointerId;

                lastMouseX =
                    event.clientX;

                lastMouseY =
                    event.clientY;


                area.classList.add(
                    "rotating");


                area.setPointerCapture(
                    event.pointerId);
            });


        area.addEventListener(
            "pointermove",
            event =>
            {
                if (!rotating ||
                    event.pointerId !==
                        rotatingPointer)
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


        const stopRotation =
            event =>
            {
                if (!rotating ||
                    event.pointerId !==
                        rotatingPointer)
                {
                    return;
                }


                rotating =
                    false;

                rotatingPointer =
                    -1;


                area.classList.remove(
                    "rotating");


                if (area.hasPointerCapture(
                        event.pointerId))
                {
                    area.releasePointerCapture(
                        event.pointerId);
                }
            };


        area.addEventListener(
            "pointerup",
            stopRotation);

        area.addEventListener(
            "pointercancel",
            stopRotation);

        area.addEventListener(
            "lostpointercapture",
            () =>
            {
                rotating =
                    false;

                rotatingPointer =
                    -1;

                area.classList.remove(
                    "rotating");
            });
    }


    function tgaPixelReader(
        bytes,
        bitsPerPixel)
    {
        let position =
            0;

        const bytesPerPixel =
            bitsPerPixel /
            8;


        return {
            setPosition(
                value)
            {
                position =
                    value;
            },


            getPosition()
            {
                return position;
            },


            read()
            {
                if (position +
                        bytesPerPixel >
                    bytes.length)
                {
                    throw new Error(
                        "Unexpected end of TGA.");
                }


                if (bitsPerPixel ===
                    8)
                {
                    const value =
                        bytes[position++];

                    return [
                        value,
                        value,
                        value,
                        255
                    ];
                }


                const blue =
                    bytes[position++];

                const green =
                    bytes[position++];

                const red =
                    bytes[position++];

                const alpha =
                    bitsPerPixel === 32
                        ? bytes[position++]
                        : 255;


                return [
                    red,
                    green,
                    blue,
                    alpha
                ];
            }
        };
    }


    function decodeTga(
        bytes)
    {
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


        const uncompressed =
            imageType === 2 ||
            imageType === 3;

        const rle =
            imageType === 10 ||
            imageType === 11;


        if (!uncompressed &&
            !rle)
        {
            throw new Error(
                "Unsupported TGA image type: " +
                imageType);
        }


        if (bitsPerPixel !== 8 &&
            bitsPerPixel !== 24 &&
            bitsPerPixel !== 32)
        {
            throw new Error(
                "Unsupported TGA pixel format: " +
                bitsPerPixel);
        }


        const reader =
            tgaPixelReader(
                bytes,
                bitsPerPixel);


        reader.setPosition(
            18 +
            idLength);


        const totalPixels =
            width *
            height;

        const pixels =
            new Array(
                totalPixels);


        if (uncompressed)
        {
            for (let index = 0;
                 index < totalPixels;
                 ++index)
            {
                pixels[index] =
                    reader.read();
            }
        }
        else
        {
            let output =
                0;


            while (output <
                   totalPixels)
            {
                const headerPosition =
                    reader.getPosition();

                if (headerPosition >=
                    bytes.length)
                {
                    throw new Error(
                        "Invalid TGA RLE data.");
                }


                const packet =
                    bytes[headerPosition];

                reader.setPosition(
                    headerPosition +
                    1);


                const count =
                    (packet & 0x7F) +
                    1;


                if ((packet & 0x80) !== 0)
                {
                    const pixel =
                        reader.read();

                    for (let index = 0;
                         index < count &&
                         output < totalPixels;
                         ++index)
                    {
                        pixels[output++] =
                            pixel;
                    }
                }
                else
                {
                    for (let index = 0;
                         index < count &&
                         output < totalPixels;
                         ++index)
                    {
                        pixels[output++] =
                            reader.read();
                    }
                }
            }
        }


        const rgba =
            new Uint8ClampedArray(
                totalPixels *
                4);


        const topOrigin =
            (descriptor & 0x20) !== 0;

        const rightOrigin =
            (descriptor & 0x10) !== 0;


        for (let index = 0;
             index < totalPixels;
             ++index)
        {
            const sourceX =
                index %
                width;

            const sourceY =
                Math.floor(
                    index /
                    width);


            const destinationX =
                rightOrigin
                    ? width -
                      sourceX -
                      1
                    : sourceX;

            const destinationY =
                topOrigin
                    ? sourceY
                    : height -
                      sourceY -
                      1;


            const destination =
                (
                    destinationY *
                    width +
                    destinationX
                ) *
                4;


            const pixel =
                pixels[index];


            rgba[destination] =
                pixel[0];

            rgba[destination + 1] =
                pixel[1];

            rgba[destination + 2] =
                pixel[2];

            rgba[destination + 3] =
                pixel[3];
        }


        return {
            width:
                width,

            height:
                height,

            data:
                rgba
        };
    }


    async function loadTga(
        canvas)
    {
        const path =
            canvas.dataset.tga;

        if (!path)
        {
            return;
        }


        try
        {
            const response =
                await fetch(
                    path,
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


            const bytes =
                new Uint8Array(
                    await response.arrayBuffer());


            const image =
                decodeTga(
                    bytes);


            canvas.width =
                image.width;

            canvas.height =
                image.height;


            const context =
                canvas.getContext(
                    "2d");


            const imageData =
                new ImageData(
                    image.data,
                    image.width,
                    image.height);


            context.putImageData(
                imageData,
                0,
                0);
        }
        catch (error)
        {
            Frontend.trace(
                "TGA UI asset failed: " +
                path +
                " : " +
                error);
        }
    }


    async function loadTgaAssets()
    {
        if (!mountedRoot)
        {
            return;
        }


        const canvases =
            Array.from(
                mountedRoot.querySelectorAll(
                    "canvas[data-tga]"));


        await Promise.all(
            canvases.map(
                canvas =>
                    loadTga(
                        canvas)));
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
                            const locale =
                                button.dataset.locale;

                            if (locale ===
                                Frontend.getLocale())
                            {
                                return;
                            }


                            await Frontend.setLocale(
                                locale);
                        });
                });
    }


    function bindActions()
    {
        element(
            "mainPlay")
            .addEventListener(
                "click",
                () =>
                {
                    if (!CharacterStore.current())
                    {
                        return;
                    }

                    Frontend.play();
                });


        element(
            "mainExit")
            .addEventListener(
                "click",
                () =>
                    Frontend.quit());


        element(
            "mainAccountId")
            .addEventListener(
                "click",
                copyAccountId);


        mountedRoot
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
    }


    async function mount(
        root)
    {
        mountedRoot =
            root;


        applyTranslations();

        renderAccount();

        renderCharacter();


        element(
            "mainVersion")
            .textContent =
                await Frontend.readVersion();


        bindLocales();

        bindActions();

        bindCharacterRotation();


        await loadTgaAssets();


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