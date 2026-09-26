"use strict";

(() =>
{
    const labels =
    {
        russian:
        {
            title: "\u041E\u0414\u0415\u0416\u0414\u0410",
            random: "\u0421\u041B\u0423\u0427\u0410\u0419\u041D\u041E",
            reset: "\u0421\u0411\u0420\u041E\u0421\u0418\u0422\u042C",
            back: "\u041D\u0410\u0417\u0410\u0414",
            choose: "\u0412\u042B\u0411\u0415\u0420\u0418\u0422\u0415",
            item: "\u041F\u0420\u0415\u0414\u041C\u0415\u0422",
            "01_jacket": "\u041A\u0423\u0420\u0422\u041A\u0418",
            "02_pants": "\u0428\u0422\u0410\u041D\u042B",
            "03_under-shirt": "\u0420\u0423\u0411\u0410\u0428\u041A\u0418",
            "04_under-pants": "\u041F\u041E\u0414\u0428\u0422\u0410\u041D\u041D\u0418\u041A\u0418",
            "05_foot": "\u041E\u0411\u0423\u0412\u042C"
        },

        english:
        {
            title: "CLOTHES",
            random: "RANDOM",
            reset: "RESET",
            back: "BACK",
            choose: "CHOOSE",
            item: "ITEM",
            "01_jacket": "JACKETS",
            "02_pants": "PANTS",
            "03_under-shirt": "SHIRTS",
            "04_under-pants": "UNDERPANTS",
            "05_foot": "FOOTWEAR"
        },

        chinese:
        {
            title: "\u670D\u88C5",
            random: "\u968F\u673A",
            reset: "\u91CD\u7F6E",
            back: "\u8FD4\u56DE",
            choose: "\u9009\u62E9",
            item: "\u7269\u54C1",
            "01_jacket": "\u5916\u5957",
            "02_pants": "\u88E4\u5B50",
            "03_under-shirt": "\u886C\u886B",
            "04_under-pants": "\u79CB\u88E4",
            "05_foot": "\u978B"
        }
    };

    const itemNames =
    {
        russian:
        {
            30100: "\u0422\u0443\u0440\u0438\u0441\u0442\u0438\u0447\u0435\u0441\u043A\u0430\u044F \u043A\u0443\u0440\u0442\u043A\u0430",
            36030: "\u041A\u0443\u0440\u0442\u043A\u0430 \u043A\u043E\u0441\u0442\u044E\u043C\u0430 \u0413\u043E\u0440\u043A\u0430-1",
            30105: "\u0428\u0442\u0430\u043D\u044B \u041C\u0430\u0431\u0443\u0442\u0430",
            36035: "\u0428\u0442\u0430\u043D\u044B \u043A\u043E\u0441\u0442\u044E\u043C\u0430 \u0413\u043E\u0440\u043A\u0430-1",
            24030: "\u0420\u0443\u0431\u0430\u0448\u043A\u0430 \u0411\u0435\u043B\u0443\u0433\u0430",
            24035: "\u041F\u043E\u0434\u0448\u0442\u0430\u043D\u043D\u0438\u043A\u0438 \u0411\u0435\u043B\u0443\u0433\u0430",
            52030: "\u041A\u0435\u0434\u044B",
            54810: "\u041A\u0440\u043E\u0441\u0441\u043E\u0432\u043A\u0438 Adidas"
        },

        english:
        {
            30100: "Tourist jacket",
            36030: "Gorka-1 jacket",
            30105: "Mabuta pants",
            36035: "Gorka-1 pants",
            24030: "Beluga shirt",
            24035: "Beluga underpants",
            52030: "Sneakers",
            54810: "Adidas shoes"
        }
    };

    let root = null;
    let groups = [];
    let selection = [];
    let selectedCategory = 0;
    let handlers = {};

    function text(key)
    {
        const table = labels[Frontend.getLocale()] || labels.russian;
        return table[key] || labels.russian[key] || String(key || "").toUpperCase();
    }

    function itemName(option)
    {
        if (option.caption) return String(option.caption);
        const locale = Frontend.getLocale();
        const table = itemNames[locale] || itemNames.english;
        return table[option.itemType] || text("item") + " " + option.itemType;
    }

    function imagePath(value)
    {
        const path = String(value || "")
            .replace(/\\/g, "/")
            .replace(/^(\.\.\/)+/, "")
            .replace(/^\/+/, "");

        return "/packs/res/" +
            path
                .split("/")
                .map(part => encodeURIComponent(part))
                .join("/");
    }

    function selectedItem(groupName)
    {
        return selection.find(value => value.group === groupName)?.itemType;
    }

    function makeNavigationButton(group, index)
    {
        const button = document.createElement("button");
        button.type = "button";
        button.classList.toggle("selected", index === selectedCategory);
        button.addEventListener("click", () =>
        {
            selectedCategory = index;
            render();
        });

        const marker = document.createElement("span");
        marker.className = "main-navigation-marker";

        const label = document.createElement("span");
        label.textContent = text(group.name);

        button.append(marker, label);
        return button;
    }

    function makeItemButton(group, option)
    {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "clothes-item";
        button.title = itemName(option);
        button.classList.toggle(
            "selected",
            Number(selectedItem(group.name)) === Number(option.itemType));

        button.addEventListener("click", () =>
        {
            if (typeof handlers.select === "function")
            {
                handlers.select(group.name, option.itemType);
            }
        });

        const image = document.createElement("img");
        image.src = imagePath(option.texture);
        image.alt = itemName(option);
        image.draggable = false;
        button.appendChild(image);
        return button;
    }

    function render()
    {
        if (!root || !groups.length) return;

        selectedCategory = Math.max(
            0,
            Math.min(selectedCategory, groups.length - 1));

        const group = groups[selectedCategory];
        const category = text(group.name);
        const nav = root.querySelector("#clothesCategories");
        const items = root.querySelector("#clothesItems");

        nav.replaceChildren(...groups.map(makeNavigationButton));
        root.querySelector("#clothesCaption").textContent =
            text("choose") + " " + category;
        items.replaceChildren(...group.options.map(option => makeItemButton(group, option)));

        const selected = group.options.find(
            option => Number(option.itemType) === Number(selectedItem(group.name)));

        root.querySelector("#clothesItemName").textContent =
            selected ? itemName(selected) : "";
    }

    function setData(nextGroups, nextSelection)
    {
        groups = Array.isArray(nextGroups) ? nextGroups : [];
        selection = Array.isArray(nextSelection) ? nextSelection : [];
        render();
    }

    function setSelection(nextSelection)
    {
        selection = Array.isArray(nextSelection) ? nextSelection : [];
        render();
    }

    function close()
    {
        if (!root) return;
        root.querySelector("#clothesScreen").hidden = true;
        root.querySelector(".creator-left").hidden = false;
    }

    function open()
    {
        if (!root || !groups.length) return;
        root.querySelector(".creator-left").hidden = true;
        root.querySelector("#appearanceScreen").hidden = true;
        root.querySelector("#clothesScreen").hidden = false;
        Frontend.openCharacterClothes();
        render();
    }

    function initialize(container, callbacks)
    {
        root = container;
        handlers = callbacks || {};

        root.querySelectorAll("[data-clothes-i18n]").forEach(
            node =>
            {
                node.textContent = text(node.dataset.clothesI18n);
            });

        root.querySelector("#clothesRandom").addEventListener(
            "click",
            () => handlers.random?.());
        root.querySelector("#clothesReset").addEventListener(
            "click",
            () => handlers.reset?.());
        root.querySelector("#clothesBack").addEventListener(
            "click",
            close);
    }

    window.CharacterClothes =
    {
        initialize,
        open,
        setData,
        setSelection
    };
})();
