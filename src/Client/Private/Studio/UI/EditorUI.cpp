#include "Studio/UI/EditorUI.h"

#include "Studio/EditorScene.h"
#include "Studio/LevelCatalog.h"
#include "Studio/UI/EditorTheme.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include "Graphics/CameraView.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <utility>

namespace
{
    bool MatchesFilter(
        const std::string& text,
        const char* filter)
    {
        if (!filter || !filter[0])
            return true;

        const std::size_t length = std::strlen(filter);

        const auto result = std::search(
            text.begin(),
            text.end(),
            filter,
            filter + length,
            [](const unsigned char a, const unsigned char b)
            {
                return std::tolower(a) == std::tolower(b);
            });

        return result != text.end();
    }
}

namespace studio::ui
{
    EditorUI::~EditorUI()
    {
        Shutdown();
    }

    bool EditorUI::Initialize(
        HWND window,
        ID3D11Device* device,
        ID3D11DeviceContext* context)
    {
        Shutdown();

        if (!window || !device || !context)
            return false;

        window_ = window;

        layoutInitialized_ =
            std::filesystem::exists("StudioLayout.ini");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = "StudioLayout.ini";

        uiScale_ = static_cast<float>(GetDpiForWindow(window)) / 96.0f;

        if (uiScale_ < 1.0f)
            uiScale_ = 1.0f;

        ImFont* font = io.Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\segoeui.ttf",
            16.0f * uiScale_);

        if (font)
            io.FontDefault = font;

        ApplyEditorTheme(uiScale_);

        if (!ImGui_ImplWin32_Init(window))
        {
            ImGui::DestroyContext();
            window_ = nullptr;
            return false;
        }

        if (!ImGui_ImplDX11_Init(device, context))
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            window_ = nullptr;
            return false;
        }

        initialized_ = true;

        AddConsoleMessage("Editor initialized.");

        return true;
    }

    bool EditorUI::ConsumeAddLightRequest(
        studio::LightType& type) noexcept
    {
        const int requested =
            std::exchange(lightTypeRequested_, -1);

        if (requested < 0 || requested > 2)
            return false;

        type = static_cast<studio::LightType>(requested);

        return true;
    }

    bool EditorUI::ConsumeSaveRequest() noexcept
    {
        return std::exchange(saveRequested_, false);
    }

    void EditorUI::Shutdown()
    {
        if (!initialized_)
            return;

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();

        initialized_ = false;
        window_ = nullptr;
    }

    void EditorUI::BeginFrame(
        EditorScene& scene,
        const LevelCatalog& catalog,
        const client::graphics::CameraView& camera,
        ID3D11ShaderResourceView* sceneTexture)
    {
        if (!initialized_)
            return;

        viewportHovered_ = false;
        viewportWheel_ = 0.0f;
        viewportRectangle_ = {};

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        BuildMainMenu();
        BuildToolbar();
        BuildDockSpace();

        if (showViewport_)
            BuildViewport(scene, camera, sceneTexture);

        if (showOutliner_)
            BuildSceneOutliner(scene);

        if (showDetails_)
            BuildDetails(scene);

        if (showContentBrowser_)
            BuildContentBrowser(catalog);

        if (showConsole_)
            BuildConsole();

        BuildOpenLevelDialog(catalog);
    }

    void EditorUI::BuildAddLightMenu()
    {
        if (ImGui::MenuItem("Directional Light"))
            lightTypeRequested_ = 2;

        if (ImGui::MenuItem("Point Light"))
            lightTypeRequested_ = 0;

        if (ImGui::MenuItem("Spot Light"))
            lightTypeRequested_ = 1;
    }

    void EditorUI::BuildToolbar()
    {
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNavFocus;

        const float toolbarHeight = 45.0f * uiScale_;

        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2(12.0f * uiScale_, 7.0f * uiScale_));

        if (ImGui::BeginViewportSideBar(
                "##EditorToolbar",
                ImGui::GetMainViewport(),
                ImGuiDir_Up,
                toolbarHeight,
                flags))
        {
            ImGui::AlignTextToFramePadding();

            ImGui::TextDisabled("STUDIO");

            ImGui::SameLine();

            ImGui::Dummy(ImVec2(15.0f * uiScale_, 1.0f));

            ImGui::SameLine();

            if (ImGui::Button("New Scene"))
                newSceneRequested_ = true;

            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::SameLine();

            if (ImGui::Button("Open Level"))
                openDialogRequested_ = true;

            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::SameLine();

            if (ImGui::Button("Refresh Levels"))
                refreshLevelsRequested_ = true;

            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::SameLine();

            if (ImGui::Button("+ Add"))
                ImGui::OpenPopup("Add Light##Toolbar");

            if (ImGui::BeginPopup("Add Light##Toolbar"))
            {
                ImGui::TextDisabled("LIGHTING");
                ImGui::Separator();

                BuildAddLightMenu();

                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::TextDisabled(" |  LEVEL EDITOR");
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorUI::Render()
    {
        if (!initialized_)
            return;

        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(
            ImGui::GetDrawData());
    }

    void EditorUI::BuildMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem(
                    "New Empty Scene", "Ctrl+N"))
                {
                    newSceneRequested_ = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem(
                    "Open Level...", "Ctrl+O"))
                {
                    openDialogRequested_ = true;
                }

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                    saveRequested_ = true;

                ImGui::MenuItem(
                    "Save As...", nullptr, false, false);

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                    exitRequested_ = true;

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem(
                    "Viewport", nullptr, &showViewport_);

                ImGui::MenuItem(
                    "Scene Outliner", nullptr, &showOutliner_);

                ImGui::MenuItem(
                    "Details", nullptr, &showDetails_);

                ImGui::MenuItem(
                    "Content Browser", nullptr, &showContentBrowser_);

                ImGui::MenuItem(
                    "Console", nullptr, &showConsole_);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Add"))
            {
                if (ImGui::BeginMenu("Light"))
                {
                    BuildAddLightMenu();
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        const ImGuiIO& io = ImGui::GetIO();

        if (io.KeyCtrl && !io.WantTextInput)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_N))
                newSceneRequested_ = true;

            if (ImGui::IsKeyPressed(ImGuiKey_O))
                openDialogRequested_ = true;

            if (ImGui::IsKeyPressed(ImGuiKey_S))
                saveRequested_ = true;
        }
    }

    void EditorUI::BuildDockSpace()
    {
        ImGuiID dockspaceId =
            ImGui::DockSpaceOverViewport(
                0,
                ImGui::GetMainViewport(),
                ImGuiDockNodeFlags_None);

        if (!layoutInitialized_)
        {
            BuildDefaultLayout(dockspaceId);
            layoutInitialized_ = true;
        }
    }

    void EditorUI::BuildDefaultLayout(
        unsigned int dockspaceId)
    {
        const ImGuiViewport* viewport =
            ImGui::GetMainViewport();

        ImGui::DockBuilderRemoveNode(dockspaceId);

        ImGui::DockBuilderAddNode(
            dockspaceId,
            ImGuiDockNodeFlags_DockSpace);

        ImGui::DockBuilderSetNodeSize(
            dockspaceId,
            viewport->Size);

        ImGuiID center = dockspaceId;

        const ImGuiID left =
            ImGui::DockBuilderSplitNode(
                center,
                ImGuiDir_Left,
                0.20f,
                nullptr,
                &center);

        const ImGuiID right =
            ImGui::DockBuilderSplitNode(
                center,
                ImGuiDir_Right,
                0.23f,
                nullptr,
                &center);

        const ImGuiID bottom =
            ImGui::DockBuilderSplitNode(
                center,
                ImGuiDir_Down,
                0.25f,
                nullptr,
                &center);

        ImGui::DockBuilderDockWindow(
            "Viewport", center);

        ImGui::DockBuilderDockWindow(
            "Scene Outliner", left);

        ImGui::DockBuilderDockWindow(
            "Details", right);

        ImGui::DockBuilderDockWindow(
            "Content Browser", bottom);

        ImGui::DockBuilderDockWindow(
            "Console", bottom);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    void EditorUI::BuildViewport(
        EditorScene& scene,
        const client::graphics::CameraView& camera,
        ID3D11ShaderResourceView* sceneTexture)
    {
        if (ImGui::Begin(
            "Viewport",
            &showViewport_,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse))
        {
            const ImVec2 size =
                ImGui::GetContentRegionAvail();

            if (size.x > 1.0f &&
                size.y > 1.0f &&
                sceneTexture)
            {
                const ImTextureID textureId =
                    static_cast<ImTextureID>(
                        reinterpret_cast<std::uintptr_t>(
                            sceneTexture));

                ImGui::Image(textureId, size);

                const ImVec2 minimum =
                    ImGui::GetItemRectMin();

                const ImVec2 maximum =
                    ImGui::GetItemRectMax();

                const ImVec2 origin =
                    ImGui::GetMainViewport()->Pos;

                viewportRectangle_ =
                {
                    static_cast<LONG>(minimum.x - origin.x),
                    static_cast<LONG>(minimum.y - origin.y),
                    static_cast<LONG>(maximum.x - origin.x),
                    static_cast<LONG>(maximum.y - origin.y)
                };

                viewportHovered_ =
                    ImGui::IsItemHovered() &&
                    !ImGui::GetIO().WantTextInput;

                if (viewportHovered_)
                {
                    viewportWheel_ =
                        ImGui::GetIO().MouseWheel;
                }

                const auto dot =
                    [](const core::math::Vector3& a,
                       const core::math::Vector3& b)
                    {
                        return
                            a.x * b.x +
                            a.y * b.y +
                            a.z * b.z;
                    };

                const auto normalize =
                    [&](core::math::Vector3 vector)
                    {
                        const float length =
                            std::sqrt(dot(vector, vector));

                        if (length > 0.000001f)
                        {
                            vector.x /= length;
                            vector.y /= length;
                            vector.z /= length;
                        }

                        return vector;
                    };

                const core::math::Vector3 forward =
                    normalize(camera.forward);

                core::math::Vector3 right =
                {
                    forward.z,
                    0.0f,
                    -forward.x
                };

                if (dot(right, right) < 0.000001f)
                    right = {1.0f, 0.0f, 0.0f};

                right = normalize(right);

                const core::math::Vector3 up =
                {
                    forward.y * right.z -
                        forward.z * right.y,

                    forward.z * right.x -
                        forward.x * right.z,

                    forward.x * right.y -
                        forward.y * right.x
                };

                constexpr float Pi =
                    3.14159265358979323846f;

                const float fov =
                    std::clamp(
                        camera.fieldOfViewDegrees,
                        20.0f,
                        90.0f) * Pi / 180.0f;

                const float tangent =
                    std::tan(fov * 0.5f);

                RECT clientRectangle{};
                GetClientRect(window_, &clientRectangle);

                const float targetWidth =
                    static_cast<float>(std::max(
                        1L,
                        clientRectangle.right -
                            clientRectangle.left));

                const float targetHeight =
                    static_cast<float>(std::max(
                        1L,
                        clientRectangle.bottom -
                            clientRectangle.top));

                const float projectionX =
                    size.x * targetHeight /
                    (2.0f * tangent * targetWidth);

                const float projectionY =
                    size.y / (2.0f * tangent);

                const auto project =
                    [&](const std::array<float, 3>& position,
                        ImVec2& screen,
                        float& depth)
                    {
                        const core::math::Vector3 relative
                        {
                            position[0] - camera.position.x,
                            position[1] - camera.position.y,
                            position[2] - camera.position.z
                        };

                        depth = dot(relative, forward);

                        if (depth <= 0.1f)
                            return false;

                        screen.x =
                            minimum.x + size.x * 0.5f +
                            dot(relative, right) *
                                projectionX / depth;

                        screen.y =
                            minimum.y + size.y * 0.5f -
                            dot(relative, up) *
                                projectionY / depth;

                        return
                            screen.x >= minimum.x &&
                            screen.x <= maximum.x &&
                            screen.y >= minimum.y &&
                            screen.y <= maximum.y;
                    };

                ImDrawList* drawList =
                    ImGui::GetWindowDrawList();

                drawList->PushClipRect(
                    minimum,
                    maximum,
                    true);

                const auto& lights = scene.Lights();

                for (std::size_t i = 0;
                     i < lights.size();
                     ++i)
                {
                    const LightObject& light = lights[i];

                    if (light.type == LightType::Directional)
                        continue;

                    ImVec2 screen{};
                    float depth = 0.0f;

                    if (!project(
                        light.position,
                        screen,
                        depth))
                    {
                        continue;
                    }

                    const bool selected =
                        scene.SelectedLightIndex() ==
                        static_cast<int>(i);

                    const ImU32 colour = light.enabled
                        ? IM_COL32(255, 205, 60, 255)
                        : IM_COL32(110, 110, 110, 255);

                    if (selected)
                    {
                        drawList->AddCircle(
                            screen,
                            13.0f,
                            IM_COL32(255, 255, 255, 255),
                            24,
                            2.0f);
                    }

                    drawList->AddCircleFilled(
                        screen,
                        8.0f,
                        colour,
                        20);

                    drawList->AddCircle(
                        screen,
                        8.0f,
                        IM_COL32(20, 20, 20, 255),
                        20,
                        1.5f);

                    if (light.type == LightType::Spot)
                    {
                        const float directionLength =
                            std::sqrt(
                                light.direction[0] *
                                    light.direction[0] +
                                light.direction[1] *
                                    light.direction[1] +
                                light.direction[2] *
                                    light.direction[2]);

                        if (directionLength > 0.000001f)
                        {
                            const float length =
                                std::min(
                                    light.radius * 0.3f,
                                    20.0f);

                            std::array<float, 3> endpoint{};

                            for (int axis = 0; axis < 3; ++axis)
                            {
                                endpoint[axis] =
                                    light.position[axis] +
                                    light.direction[axis] /
                                        directionLength * length;
                            }

                            ImVec2 endpointScreen{};
                            float endpointDepth = 0.0f;

                            if (project(
                                endpoint,
                                endpointScreen,
                                endpointDepth))
                            {
                                drawList->AddLine(
                                    screen,
                                    endpointScreen,
                                    colour,
                                    2.0f);
                            }
                        }
                    }

                    ImGui::PushID(static_cast<int>(i));

                    ImGui::SetCursorScreenPos(
                        ImVec2(
                            screen.x - 12.0f,
                            screen.y - 12.0f));

                    ImGui::InvisibleButton(
                        "##LightHandle",
                        ImVec2(24.0f, 24.0f));

                    if (ImGui::IsItemHovered())
                        ImGui::SetMouseCursor(
                            ImGuiMouseCursor_Hand);

                    if (ImGui::IsItemClicked(
                        ImGuiMouseButton_Left))
                    {
                        scene.SelectLight(static_cast<int>(i));
                    }

                    if (ImGui::IsItemActive() &&
                        ImGui::IsMouseDragging(
                            ImGuiMouseButton_Left))
                    {
                        LightObject updated =
                            scene.Lights()[i];

                        const ImVec2 delta =
                            ImGui::GetIO().MouseDelta;

                        const float movementX =
                            delta.x * depth / projectionX;

                        const float movementY =
                            -delta.y * depth / projectionY;

                        updated.position[0] +=
                            right.x * movementX +
                            up.x * movementY;

                        updated.position[1] +=
                            right.y * movementX +
                            up.y * movementY;

                        updated.position[2] +=
                            right.z * movementX +
                            up.z * movementY;

                        scene.UpdateLight(i, updated);
                    }

                    ImGui::PopID();
                }

                drawList->PopClipRect();
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildSceneOutliner(
        EditorScene& scene)
    {
        if (ImGui::Begin(
            "Scene Outliner",
            &showOutliner_))
        {
            ImGui::TextUnformatted(
                scene.Name().c_str());

            ImGui::Separator();

            if (!scene.Directory().empty())
            {
                if (ImGui::TreeNode("Static geometry"))
                {
                    ImGui::Text(
                        "Meshes: %zu",
                        scene.Statistics().meshes);

                    ImGui::Text(
                        "Visuals: %zu",
                        scene.Statistics().visuals);

                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::TextDisabled("No static geometry.");
            }

            ImGui::Spacing();

            ImGui::SetNextItemOpen(
                true,
                ImGuiCond_Once);

            if (ImGui::TreeNode("Lighting"))
            {
                const bool environmentSelected =
                    scene.SelectedLightIndex() == -2;

                if (ImGui::Selectable(
                    "Environment",
                    environmentSelected))
                {
                    scene.SelectLight(-2);
                }

                const auto& lights = scene.Lights();

                for (std::size_t i = 0;
                     i < lights.size();
                     ++i)
                {
                    ImGui::PushID(static_cast<int>(i));

                    const bool selected =
                        scene.SelectedLightIndex() ==
                        static_cast<int>(i);

                    const std::string& name =
                        lights[i].name;

                    if (ImGui::Selectable(
                        name.c_str(),
                        selected))
                    {
                        scene.SelectLight(
                            static_cast<int>(i));
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    bool EditorUI::BuildLightDetails(
        EditorScene& scene)
    {
        if (scene.SelectedLightIndex() == -2)
        {
            ImGui::TextUnformatted("Environment");
            ImGui::Separator();

            std::array<float, 3> ambient =
                scene.Ambient();

            if (ImGui::ColorEdit3(
                "Ambient Color",
                ambient.data()))
            {
                scene.SetAmbient(ambient);
            }

            if (ImGui::DragFloat3(
                "Ambient RGB",
                ambient.data(),
                0.005f,
                0.0f,
                10.0f))
            {
                scene.SetAmbient(ambient);
            }

            return true;
        }

        const LightObject* selected =
            scene.SelectedLight();

        if (!selected)
            return false;

        const int selectedIndex =
            scene.SelectedLightIndex();

        LightObject light = *selected;

        bool changed = false;

        char name[128]{};

        std::snprintf(
            name,
            sizeof(name),
            "%s",
            light.name.c_str());

        ImGui::TextUnformatted("Light Component");
        ImGui::Separator();

        if (ImGui::InputText(
            "Name",
            name,
            sizeof(name)))
        {
            light.name = name;
            changed = true;
        }

        changed |= ImGui::Checkbox(
            "Enabled",
            &light.enabled);

        const char* typeName = "Point Light";

        if (light.type == LightType::Spot)
            typeName = "Spot Light";

        if (light.type == LightType::Directional)
            typeName = "Directional Light";

        ImGui::Text("Type: %s", typeName);

        ImGui::SeparatorText("Transform");

        if (light.type != LightType::Directional)
        {
            changed |= ImGui::DragFloat3(
                "Location",
                light.position.data(),
                0.5f);
        }

        if (light.type != LightType::Point)
        {
            changed |= ImGui::DragFloat3(
                "Direction",
                light.direction.data(),
                0.01f,
                -1.0f,
                1.0f);
        }

        ImGui::SeparatorText("Light");

        changed |= ImGui::ColorEdit3(
            "Light Color",
            light.colour.data());

        changed |= ImGui::DragFloat(
            "Intensity",
            &light.intensity,
            0.1f,
            0.0f,
            1000.0f,
            "%.2f");

        if (light.type != LightType::Directional)
        {
            changed |= ImGui::DragFloat(
                "Attenuation Radius",
                &light.radius,
                1.0f,
                0.1f,
                100000.0f,
                "%.2f");

            changed |= ImGui::DragFloat(
                "Inner Radius",
                &light.innerRadius,
                0.5f,
                0.0f,
                light.radius,
                "%.2f");

            changed |= ImGui::Checkbox(
                "Specular",
                &light.specular);
        }

        if (light.type == LightType::Spot)
        {
            changed |= ImGui::SliderFloat(
                "Cone Half-Angle",
                &light.coneAngleDegrees,
                1.0f,
                89.0f,
                "%.1f deg");
        }

        if (changed)
        {
            scene.UpdateLight(
                static_cast<std::size_t>(selectedIndex),
                light);
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Delete Light"))
        {
            scene.RemoveSelectedLight();
        }

        return true;
    }

    void EditorUI::BuildDetails(
        const EditorScene& scene)
    {
        if (ImGui::Begin("Details", &showDetails_))
        {
            if (BuildLightDetails(scene))
            {
            }
            else if (scene.IsEmpty())
            {
                ImGui::TextDisabled(
                    "No object selected.");
            }
            else
            {
                const SceneStatistics& stats =
                    scene.Statistics();

                ImGui::Text(
                    "Level: %s", scene.Name().c_str());

                ImGui::Separator();

                ImGui::Text(
                    "Visuals: %zu", stats.visuals);

                ImGui::Text(
                    "Meshes: %zu", stats.meshes);

                ImGui::Text(
                    "Vertices: %zu", stats.vertices);

                ImGui::Text(
                    "Triangles: %zu", stats.triangles);

                ImGui::Text(
                    "Loaded textures: %zu", stats.textures);

                ImGui::Text(
                    "Missing textures: %zu",
                    stats.missingTextures);

                ImGui::Separator();
                ImGui::Text("HOM: %s", !stats.homPresent ? "Not present" :
                    (stats.homTriangles == 0 ? "Empty" : "Enabled"));
                if (stats.homPresent)
                {
                    ImGui::Text("HOM triangles: %zu", stats.homTriangles);
                    ImGui::Text("HOM tested: %zu", stats.homTestedVisuals);
                    ImGui::Text("HOM culled: %zu", stats.homCulledVisuals);
                }

                ImGui::Separator();

                ImGui::Text("CFORM: %s", !stats.cformPresent ? "Not present" :
                    (stats.collisionFaces == 0 ? "Empty" : "Loaded (indexed)"));
                if (stats.cformPresent)
                {
                    ImGui::Text("Collision vertices: %u", stats.collisionVertices);
                    ImGui::Text("Collision faces: %u", stats.collisionFaces);
                    ImGui::Text("BVH nodes: %zu", stats.collisionNodes);
                    ImGui::Text("Material IDs: %zu", stats.collisionMaterials);
                    ImGui::Text("Sector IDs: %zu", stats.collisionSectors);
                    ImGui::Text("Mapped file: %.1f MiB", double(stats.collisionMappedBytes)/1048576.0);
                    ImGui::Text("BVH memory: %.1f MiB", double(stats.collisionIndexBytes)/1048576.0);
                    ImGui::TextDisabled("Mapped pages are managed by Windows.");
                    ImGui::BeginDisabled(stats.collisionFaces == 0);
                    if (ImGui::Button("Probe collision (view centre)"))
                        collisionProbeRequested_ = true;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Cast along the camera forward direction. Free camera is unchanged.");
                    ImGui::EndDisabled();
                    const auto& probe = stats.collisionProbe;
                    if (probe.performed)
                    {
                        if (!probe.hit) ImGui::TextDisabled("Probe: no collision hit.");
                        else
                        {
                            ImGui::Text("Hit face: %u | distance: %.2f", probe.face, probe.distance);
                            ImGui::Text("Physical material ID: %u", unsigned(probe.material));
                            if (probe.sector == 0xffffu) ImGui::Text("Sector: unknown (0xffff)");
                            else ImGui::Text("Sector: %u", unsigned(probe.sector));
                            ImGui::Text("Position: %.2f %.2f %.2f", probe.position.x, probe.position.y, probe.position.z);
                            ImGui::Text("Normal: %.3f %.3f %.3f", probe.normal.x, probe.normal.y, probe.normal.z);
                            ImGui::Text("Suppress shadows: %s", probe.suppressShadows ? "yes" : "no");
                            ImGui::Text("Suppress wallmarks: %s", probe.suppressWallmarks ? "yes" : "no");
                        }
                    }
                }
                ImGui::Separator();

                const std::string path =
                    PathToUtf8(scene.Directory());

                ImGui::TextWrapped(
                    "%s", path.c_str());
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildContentBrowser(
        const LevelCatalog& catalog)
    {
        if (ImGui::Begin(
            "Content Browser",
            &showContentBrowser_))
        {
            if (ImGui::Button("Open Level"))
                openDialogRequested_ = true;

            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::SameLine();

            if (ImGui::Button("Refresh"))
                refreshLevelsRequested_ = true;

            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::SameLine();

            ImGui::TextDisabled(
                "Levels: %zu",
                catalog.Entries().size());

            ImGui::Separator();

            ImGui::SetNextItemWidth(-1.0f);

            ImGui::InputTextWithHint(
                "##ContentSearch",
                "Search level assets...",
                contentSearch_,
                sizeof(contentSearch_));

            ImGui::Spacing();

            if (ImGui::BeginChild(
                    "##ContentItems",
                    ImVec2(0.0f, 0.0f)))
            {
                const auto& levels = catalog.Entries();

                for (std::size_t i = 0; i < levels.size(); ++i)
                {
                    const auto& level = levels[i];

                    if (!MatchesFilter(
                            level.name,
                            contentSearch_))
                    {
                        continue;
                    }

                    ImGui::PushID(static_cast<int>(i));

                    const bool selected =
                        contentSelectedIndex_ ==
                        static_cast<int>(i);

                    if (ImGui::Selectable(
                            level.name.c_str(),
                            selected,
                            ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        contentSelectedIndex_ =
                            static_cast<int>(i);

                        if (ImGui::IsMouseDoubleClicked(
                                ImGuiMouseButton_Left))
                        {
                            requestedLevel_ = level.directory;
                            openLevelRequested_ = true;
                        }
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetMouseCursor(
                            ImGuiMouseCursor_Hand);

                        ImGui::SetTooltip(
                            "%s",
                            level.name.c_str());
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }

    void EditorUI::BuildConsole()
    {
        if (ImGui::Begin("Console", &showConsole_))
        {
            for (const std::string& line : consoleLines_)
            {
                ImGui::TextWrapped(
                    "%s", line.c_str());
            }

            if (scrollConsole_)
            {
                ImGui::SetScrollHereY(1.0f);
                scrollConsole_ = false;
            }
        }

        ImGui::End();
    }

    void EditorUI::BuildOpenLevelDialog(
        const LevelCatalog& catalog)
    {
        if (openDialogRequested_)
        {
            selectedLevelIndex_ = -1;
            levelSearch_[0] = '\0';

            ImGui::OpenPopup("Open Level");

            openDialogRequested_ = false;
        }

        ImGuiViewport* viewport =
            ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(
            viewport->GetCenter(),
            ImGuiCond_Appearing,
            ImVec2(0.5f, 0.5f));

        ImGui::SetNextWindowSize(
            ImVec2(
                660.0f * uiScale_,
                540.0f * uiScale_),
            ImGuiCond_Appearing);

        if (!ImGui::BeginPopupModal(
                "Open Level",
                nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse))
        {
            return;
        }

        ImGui::TextDisabled(
            "CONTENT BROWSER / GAMEDATA / LEVELS");

        ImGui::Spacing();

        ImGui::TextUnformatted("Select Level");

        ImGui::SameLine();

        ImGui::TextDisabled(
            "(%zu available)",
            catalog.Entries().size());

        ImGui::Separator();

        ImGui::SetNextItemWidth(-1.0f);

        ImGui::InputTextWithHint(
            "##LevelSearch",
            "Search levels...",
            levelSearch_,
            sizeof(levelSearch_));

        ImGui::Spacing();

        const auto& levels = catalog.Entries();

        if (catalog.Roots().empty())
        {
            ImGui::TextDisabled(
                "gamedata/levels directory not found.");
        }
        else if (levels.empty())
        {
            ImGui::TextDisabled(
                "No valid X-Ray levels found.");
        }

        if (selectedLevelIndex_ >=
            static_cast<int>(levels.size()))
        {
            selectedLevelIndex_ = -1;
        }

        if (ImGui::BeginChild(
                "##LevelList",
                ImVec2(
                    0.0f,
                    305.0f * uiScale_),
                ImGuiChildFlags_Borders))
        {
            bool anyVisible = false;

            for (std::size_t i = 0; i < levels.size(); ++i)
            {
                const auto& level = levels[i];

                if (!MatchesFilter(
                        level.name,
                        levelSearch_))
                {
                    continue;
                }

                anyVisible = true;

                ImGui::PushID(static_cast<int>(i));

                const bool selected =
                    selectedLevelIndex_ ==
                    static_cast<int>(i);

                if (ImGui::Selectable(
                        level.name.c_str(),
                        selected,
                        ImGuiSelectableFlags_AllowDoubleClick))
                {
                    selectedLevelIndex_ =
                        static_cast<int>(i);

                    if (ImGui::IsMouseDoubleClicked(
                            ImGuiMouseButton_Left))
                    {
                        requestedLevel_ = level.directory;
                        openLevelRequested_ = true;

                        ImGui::CloseCurrentPopup();
                    }
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetMouseCursor(
                        ImGuiMouseCursor_Hand);
                }

                ImGui::PopID();
            }

            if (!anyVisible)
            {
                ImGui::TextDisabled(
                    "No matching levels.");
            }
        }

        ImGui::EndChild();

        ImGui::Spacing();

        const bool canOpen =
            selectedLevelIndex_ >= 0 &&
            selectedLevelIndex_ <
                static_cast<int>(levels.size());

        if (canOpen)
        {
            const std::string path =
                PathToUtf8(
                    levels[selectedLevelIndex_].directory);

            ImGui::TextDisabled("Selected:");

            ImGui::SameLine();

            ImGui::TextWrapped("%s", path.c_str());
        }
        else
        {
            ImGui::TextDisabled(
                "Select a level to continue.");
        }

        ImGui::Separator();

        const float buttonWidth = 125.0f * uiScale_;

        ImGui::BeginDisabled(!canOpen);

        if (ImGui::Button(
                "Open",
                ImVec2(buttonWidth, 0.0f)) &&
            canOpen)
        {
            requestedLevel_ =
                levels[selectedLevelIndex_].directory;

            openLevelRequested_ = true;

            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button(
                "Refresh",
                ImVec2(buttonWidth, 0.0f)))
        {
            refreshLevelsRequested_ = true;
        }

        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImGui::SameLine();

        if (ImGui::Button(
                "Cancel",
                ImVec2(buttonWidth, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImGui::EndPopup();
    }

    void EditorUI::AddConsoleMessage(
        std::string message)
    {
        consoleLines_.push_back(
            std::move(message));

        if (consoleLines_.size() > 500)
            consoleLines_.erase(consoleLines_.begin());

        scrollConsole_ = true;
    }

    bool EditorUI::ConsumeNewSceneRequest() noexcept
    {
        return std::exchange(newSceneRequested_, false);
    }

    bool EditorUI::ConsumeOpenLevelRequest(
        std::filesystem::path& directory)
    {
        if (!std::exchange(openLevelRequested_, false))
            return false;

        directory = std::move(requestedLevel_);
        requestedLevel_.clear();

        return true;
    }

    bool EditorUI::ConsumeRefreshLevelsRequest() noexcept
    {
        return std::exchange(refreshLevelsRequested_, false);
    }

    bool EditorUI::ConsumeExitRequest() noexcept
    {
        return std::exchange(exitRequested_, false);
    }

    bool EditorUI::ConsumeCollisionProbeRequest() noexcept
    {
        return std::exchange(collisionProbeRequested_, false);
    }

    bool EditorUI::ViewportHovered() const noexcept
    {
        return viewportHovered_;
    }

    float EditorUI::ViewportWheel() const noexcept
    {
        return viewportWheel_;
    }

    const RECT& EditorUI::ViewportRectangle() const noexcept
    {
        return viewportRectangle_;
    }
}
