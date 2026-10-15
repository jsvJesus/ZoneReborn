#include "Engine/Core/Engine.hpp"
#include "WorldEditor/WorldEditor.hpp"

int main() {
    engine::initialize();
    world_editor::initialize();
    return 0;
}
