#include "Editor/Viewport/CameraNavigation.h"

#include <algorithm>
#include <cmath>

namespace editor::viewport {

    void update_camera_navigation(
        const CameraNavigationState camera,
        const CameraNavigationSettings& settings,
        const ImGuiIO& io
    ) {
        const float orbitSensitivity =
            0.005f * settings.orbitSpeed;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            camera.yaw +=
                io.MouseDelta.x * orbitSensitivity;

            camera.pitch = std::clamp(
                camera.pitch -
                    io.MouseDelta.y * orbitSensitivity,
                0.1f,
                1.48f
            );
        }

        const float rightX = std::cos(camera.yaw);
        const float rightZ = -std::sin(camera.yaw);

        const float forwardX = -std::sin(camera.yaw);
        const float forwardZ = -std::cos(camera.yaw);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            float speed =
                camera.distance * 0.0015f * settings.panSpeed;

            if (io.KeyShift) {
                speed *= 2.5f;
            } else if (io.KeyCtrl) {
                speed *= 0.25f;
            }

            camera.targetX -= io.MouseDelta.x * rightX * speed;
            camera.targetZ -= io.MouseDelta.x * rightZ * speed;

            camera.targetX -= io.MouseDelta.y * forwardX * speed;
            camera.targetZ -= io.MouseDelta.y * forwardZ * speed;
        }

        if (io.MouseWheel != 0.0f) {
            const float factor = std::exp(
                -0.18f * settings.zoomSpeed * io.MouseWheel
            );

            camera.distance = std::clamp(
                camera.distance * factor,
                10.0f,
                30000.0f
            );
        }

        if (io.WantTextInput ||
            ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
            ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            return;
        }

        float axisX = 0.0f;
        float axisZ = 0.0f;

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            axisZ += 1.0f;
        }

        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            axisZ -= 1.0f;
        }

        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            axisX += 1.0f;
        }

        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            axisX -= 1.0f;
        }

        const float length = std::hypot(axisX, axisZ);

        if (length <= 0.0f) {
            return;
        }

        axisX /= length;
        axisZ /= length;

        float speed = std::clamp(
            camera.distance * 0.8f,
            50.0f,
            1800.0f
        ) * settings.panSpeed;

        if (io.KeyShift) {
            speed *= 3.0f;
        } else if (io.KeyCtrl) {
            speed *= 0.25f;
        }

        const float distance =
            speed * std::clamp(io.DeltaTime, 0.0f, 0.1f);

        camera.targetX += (
            rightX * axisX + forwardX * axisZ
        ) * distance;

        camera.targetZ += (
            rightZ * axisX + forwardZ * axisZ
        ) * distance;
    }

}
