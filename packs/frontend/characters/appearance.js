"use strict";

(() =>
{
    const CONFIG_PATH = "/packs/res/scripts/common/data/charMakerFaceCfg.json";
    const IMAGE_ROOT = "/packs/res/soGUI/frame/character_creation/face/";

    const labels =
    {
        russian: { title: "ВНЕШНОСТЬ", face: "ЛИЦО", skin: "КОЖА", eye: "ГЛАЗА", hairstyle: "ПРИЧЁСКА", eyebrows: "БРОВИ", mustache: "УСЫ", beard: "БОРОДА", color: "ЦВЕТ", age: "ВОЗРАСТ", details: "ДЕТАЛИ", length: "ДЛИНА", position: "ПОЛОЖЕНИЕ", rotation: "НАКЛОН", unshaven: "ЩЕТИНА", reset: "СБРОСИТЬ", random: "СЛУЧАЙНО", apply: "ПРИМЕНИТЬ", back: "НАЗАД", randomFace: "СЛУЧАЙНАЯ ФОРМА ЛИЦА" },
        english: { title: "APPEARANCE", face: "FACE", skin: "SKIN", eye: "EYES", hairstyle: "HAIRSTYLE", eyebrows: "EYEBROWS", mustache: "MUSTACHE", beard: "BEARD", color: "COLOUR", age: "AGE", details: "DETAILS", length: "LENGTH", position: "POSITION", rotation: "ROTATION", unshaven: "STUBBLE", reset: "RESET", random: "RANDOM", apply: "APPLY", back: "BACK", randomFace: "RANDOM FACE FORM" },
        chinese: { title: "外观", face: "脸型", skin: "皮肤", eye: "眼睛", hairstyle: "发型", eyebrows: "眉毛", mustache: "胡子", beard: "胡须", color: "颜色", age: "年龄", details: "细节", length: "长度", position: "位置", rotation: "角度", unshaven: "胡茬", reset: "重置", random: "随机", apply: "应用", back: "返回", randomFace: "随机脸型" }
    };

    let root = null;
    let categories = [];
    let selected = 0;
    let state = null;
    let ready = false;

    function text(key)
    {
        const table = labels[Frontend.getLocale()] || labels.russian;
        return table[key] || labels.russian[key] || String(key || "").toUpperCase();
    }

    function imagePath(value)
    {
        const name = String(value || "").replace(/\\/g, "/").split("/").pop();
        return IMAGE_ROOT + encodeURIComponent(name);
    }

    function collectControls(value, output)
    {
        if (!value) return;
        if (Array.isArray(value))
        {
            value.forEach(item => collectControls(item, output));
            return;
        }
        if (typeof value !== "object") return;
        if (value.donat_only) return;
        if (value.type === "Slider" || value.type === "ColorList" || value.type === "PictureButtonList")
        {
            output.push(value);
            return;
        }
        if (value.type === "None" && value.id === "None")
        {
            output.push({ type: "FaceForm", id: "FaceForm", label: "face" });
        }
        if (Array.isArray(value.data)) value.data.forEach(item => collectControls(item, output));
    }

    async function loadConfig()
    {
        const response = await fetch(CONFIG_PATH, { cache: "no-store" });
        if (!response.ok) throw new Error("HTTP " + response.status);
        const config = await response.json();
        categories = [];
        Object.keys(config).sort().forEach(key =>
        {
            const source = Array.isArray(config[key]) ? config[key] : [config[key]];
            source.forEach((value, index) =>
            {
                if (!value || value.donat_only) return;
                if (value.label === "eye") return;
                const controls = [];
                collectControls(value, controls);
                const visibleControls = controls.filter(control => control.type !== "Slider");
                if (visibleControls.length)
                {
                    categories.push({ key: key + "_" + index, label: value.label || key.replace(/^\d+_/, ""), controls: visibleControls });
                }
            });
        });
    }

    function makeButton(className, title, click)
    {
        const button = document.createElement("button");
        button.type = "button";
        button.className = className;
        button.title = title;
        button.addEventListener("click", click);
        return button;
    }

    function send(id, value)
    {
        if (!ready) return;
        Frontend.setCharacterFaceValue(id, value);
    }

    function renderControl(control)
    {
        const wrapper = document.createElement("div");
        const title = document.createElement("div");
        title.className = "appearance-control-title";
        title.textContent = text(control.label || control.id);
        wrapper.appendChild(title);

        if (control.type === "FaceForm")
        {
            const button = makeButton("appearance-random-face", text("randomFace"), () => Frontend.randomizeCharacterFace());
            button.textContent = text("randomFace");
            wrapper.appendChild(button);
        }
        else if (control.type === "PictureButtonList")
        {
            const list = document.createElement("div");
            list.className = "appearance-pictures";
            (control.data || []).filter(option => !option.donat_only).forEach(option =>
            {
                const value = Number(option.item_id);
                const button = makeButton("appearance-picture", String(value), () => send(control.id, value));
                button.dataset.faceId = control.id;
                button.dataset.faceValue = String(value);
                const image = document.createElement("img");
                image.src = imagePath(option.image);
                image.alt = "";
                button.appendChild(image);
                list.appendChild(button);
            });
            wrapper.appendChild(list);
        }
        else if (control.type === "ColorList")
        {
            const list = document.createElement("div");
            list.className = "appearance-swatches";
            (control.colors || []).forEach(entry =>
            {
                const hex = String(entry).split(":")[0].replace("#", "");
                const value = Number.parseInt(hex, 16);
                const button = makeButton("appearance-swatch", "#" + hex, () => send(control.id, value));
                button.style.backgroundColor = "#" + hex;
                button.dataset.faceId = control.id;
                button.dataset.faceValue = String(value);
                list.appendChild(button);
            });
            wrapper.appendChild(list);
        }
        else if (control.type === "Slider")
        {
            const row = document.createElement("div");
            row.className = "appearance-slider-row";
            const slider = document.createElement("input");
            slider.type = "range";
            slider.min = "0";
            slider.max = "100";
            slider.step = "1";
            slider.className = "appearance-slider";
            slider.dataset.faceId = control.id;
            slider.value = state && state[control.id] !== undefined ? state[control.id] : 0;
            const value = document.createElement("span");
            value.className = "appearance-slider-value";
            value.textContent = slider.value + "%";
            let frame = 0;
            slider.addEventListener("input", () =>
            {
                value.textContent = slider.value + "%";
                if (frame) cancelAnimationFrame(frame);
                frame = requestAnimationFrame(() => { frame = 0; send(control.id, slider.value); });
            });
            row.append(slider, value);
            wrapper.appendChild(row);
        }
        return wrapper;
    }

    function updateSelection()
    {
        if (!root || !state) return;
        root.querySelectorAll("[data-face-id][data-face-value]").forEach(button =>
            button.classList.toggle("selected", Number(button.dataset.faceValue) === Number(state[button.dataset.faceId])));
        root.querySelectorAll("input[data-face-id]").forEach(slider =>
        {
            if (state[slider.dataset.faceId] === undefined) return;
            slider.value = state[slider.dataset.faceId];
            const label = slider.parentElement.querySelector(".appearance-slider-value");
            if (label) label.textContent = slider.value + "%";
        });
    }

    function renderCategory()
    {
        const category = categories[selected];
        if (!category) return;
        root.querySelector("#appearanceCaption").textContent = text(category.label);
        const controls = root.querySelector("#appearanceControls");
        controls.replaceChildren(...category.controls.map(renderControl));
        root.querySelectorAll("#appearanceCategories button").forEach((button, index) => button.classList.toggle("selected", index === selected));
        updateSelection();
    }

    function renderCategories()
    {
        const nav = root.querySelector("#appearanceCategories");
        nav.replaceChildren(...categories.map((category, index) =>
        {
            const button = makeButton("", text(category.label), () => { selected = index; renderCategory(); });
            const marker = document.createElement("span");
            marker.className = "main-navigation-marker";
            const label = document.createElement("span");
            label.textContent = text(category.label);
            button.append(marker, label);
            return button;
        }));
        renderCategory();
    }

    function close(apply)
    {
        if (!root) return;
        apply ? Frontend.applyCharacterFace() : Frontend.cancelCharacterFace();
        root.querySelector("#appearanceScreen").hidden = true;
        root.querySelector(".creator-left").hidden = false;
        ready = false;
    }

    async function initialize(container)
    {
        root = container;
        root.querySelectorAll("[data-face-i18n]").forEach(node => node.textContent = text(node.dataset.faceI18n));
        root.querySelector("#appearanceReset").addEventListener("click", () => Frontend.resetCharacterFace());
        root.querySelector("#appearanceRandom").addEventListener("click", () => Frontend.randomizeCharacterFace());
        root.querySelector("#appearanceApply").addEventListener("click", () => close(true));
        root.querySelector("#appearanceBack").addEventListener("click", () => close(false));
        try { await loadConfig(); renderCategories(); }
        catch (error) { Frontend.trace("Face config failed: " + error); }
    }

    function open()
    {
        if (!root || !categories.length) return;
        root.querySelector(".creator-left").hidden = true;
        root.querySelector("#appearanceScreen").hidden = false;
        ready = false;
        Frontend.openCharacterFace();
    }

    function receiveState(value)
    {
        state = value || {};
        ready = true;
        updateSelection();
    }

    window.CharacterAppearance = { initialize, open, receiveState };
})();
