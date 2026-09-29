#include "Character/CharacterAnimationCatalog.h"

#include "Core/Log.h"
#include "Core/Resources/ResourceType.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    constexpr char VerifiedIdlePath[] =
        "res/characters2/basemodel/animations/unarmed/"
        "idle_unarmed/idle_move_stay_unarmed.animation";

    std::string Lower(
        std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](
                const unsigned char character)
            {
                return
                    static_cast<char>(
                        std::tolower(
                            character));
            });

        return value;
    }

    bool Contains(
        const std::string& value,
        const std::string_view token)
    {
        return
            value.find(
                token) !=
            std::string::npos;
    }

    bool IsInjuredVariant(
        const std::string& path)
    {
        return
            Contains(
                path,
                "injured") ||
            Contains(
                path,
                "wounded") ||
            Contains(
                path,
                "wound_") ||
            Contains(
                path,
                "limp") ||
            Contains(
                path,
                "cripple");
    }

    bool IsLocomotionTransition(
        const std::string& path)
    {
        return
            Contains(
                path,
                "stop_") ||
            Contains(
                path,
                "_stop") ||
            Contains(
                path,
                "start_") ||
            Contains(
                path,
                "_start");
    }

    int CommonScore(
        const std::string& path)
    {
        int score =
            0;

        if (Contains(
                path,
                "characters2/basemodel"))
        {
            score +=
                500;
        }

        if (Contains(
                path,
                "/animations/"))
        {
            score +=
                250;
        }

        if (Contains(
                path,
                "/unarmed/"))
        {
            score +=
                700;
        }
        else if (Contains(
                     path,
                     "unarmed"))
        {
            score +=
                450;
        }

        if (Contains(
                path,
                "stay_unarmed"))
        {
            score +=
                500;
        }

        if (Contains(
                path,
                "weapon") ||
            Contains(
                path,
                "rifle") ||
            Contains(
                path,
                "pistol") ||
            Contains(
                path,
                "carabine") ||
            Contains(
                path,
                "shotgun") ||
            Contains(
                path,
                "melee"))
        {
            score -=
                1000;
        }

        if (Contains(
                path,
                "incombat"))
        {
            score -=
                250;
        }

        if (Contains(
                path,
                "aim"))
        {
            score -=
                300;
        }

        return score;
    }

    int ScoreIdle(
        const std::string& path)
    {
        if (!Contains(
                path,
                "idle"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "death") ||
            Contains(
                path,
                "crawl") ||
            Contains(
                path,
                "crouch"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "stay"))
        {
            score +=
                500;
        }

        if (Contains(
                path,
                "move_stay"))
        {
            score +=
                900;
        }

        if (Contains(
                path,
                "idle_move_stay_unarmed"))
        {
            score +=
                2000;
        }

        return score;
    }

    int ScoreWalk(
        const std::string& path)
    {
        if (!Contains(
                path,
                "walk"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path) ||
            IsLocomotionTransition(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "crawl") ||
            Contains(
                path,
                "crouch") ||
            Contains(
                path,
                "death") ||
            Contains(
                path,
                "sprint") ||
            Contains(
                path,
                "swim") ||
            Contains(
                path,
                "ladder"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "walk_forwardr_stay_unarmed"))
        {
            score +=
                3000;
        }
        else if (Contains(
                     path,
                     "walk_forward_stay_unarmed"))
        {
            score +=
                2800;
        }
        else if (Contains(
                     path,
                     "walk_forward"))
        {
            score +=
                1500;
        }
        else if (Contains(
                     path,
                     "forward"))
        {
            score +=
                900;
        }

        if (Contains(
                path,
                "stay_unarmed"))
        {
            score +=
                700;
        }

        if (Contains(
                path,
                "back"))
        {
            score -=
                1200;
        }

        if (Contains(
                path,
                "strafe"))
        {
            score -=
                1200;
        }

        if (Contains(
                path,
                "left") ||
            Contains(
                path,
                "right"))
        {
            score -=
                800;
        }

        return score;
    }

    int ScoreRun(
        const std::string& path)
    {
        if (!Contains(
                path,
                "run"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path) ||
            IsLocomotionTransition(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "crawl") ||
            Contains(
                path,
                "crouch") ||
            Contains(
                path,
                "death") ||
            Contains(
                path,
                "sprint") ||
            Contains(
                path,
                "swim") ||
            Contains(
                path,
                "ladder"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "run_forwardr_stay_unarmed"))
        {
            score +=
                3000;
        }
        else if (Contains(
                     path,
                     "run_forward_stay_unarmed"))
        {
            score +=
                2800;
        }
        else if (Contains(
                     path,
                     "run_forward"))
        {
            score +=
                1500;
        }
        else if (Contains(
                     path,
                     "forward"))
        {
            score +=
                900;
        }

        if (Contains(
                path,
                "stay_unarmed"))
        {
            score +=
                700;
        }

        if (Contains(
                path,
                "back"))
        {
            score -=
                1200;
        }

        if (Contains(
                path,
                "strafe"))
        {
            score -=
                1200;
        }

        if (Contains(
                path,
                "left") ||
            Contains(
                path,
                "right"))
        {
            score -=
                800;
        }

        return score;
    }

    int ScoreSprint(
        const std::string& path)
    {
        if (!Contains(
                path,
                "sprint"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path) ||
            IsLocomotionTransition(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "crawl") ||
            Contains(
                path,
                "crouch") ||
            Contains(
                path,
                "death") ||
            Contains(
                path,
                "swim") ||
            Contains(
                path,
                "ladder") ||
            Contains(
                path,
                "_to_"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "sprint_forwardr_stay_unarmed"))
        {
            score +=
                3500;
        }
        else if (Contains(
                     path,
                     "sprint_forward_stay_unarmed"))
        {
            score +=
                3300;
        }
        else if (Contains(
                     path,
                     "sprint_forward"))
        {
            score +=
                2000;
        }
        else if (Contains(
                     path,
                     "forward"))
        {
            score +=
                1000;
        }

        if (Contains(
                path,
                "stay_unarmed"))
        {
            score +=
                700;
        }

        if (Contains(
                path,
                "back"))
        {
            score -=
                1500;
        }

        if (Contains(
                path,
                "strafe"))
        {
            score -=
                1500;
        }

        if (Contains(
                path,
                "left") ||
            Contains(
                path,
                "right"))
        {
            score -=
                900;
        }

        return
            score;
    }

    int ScoreCrouchIdle(
        const std::string& path)
    {
        if (!Contains(
                path,
                "crouch"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path) ||
            IsLocomotionTransition(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "_to_") ||
            Contains(
                path,
                "crawl") ||
            Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "death") ||
            Contains(
                path,
                "sprint") ||
            Contains(
                path,
                "walk") ||
            Contains(
                path,
                "run") ||
            Contains(
                path,
                "forward") ||
            Contains(
                path,
                "back") ||
            Contains(
                path,
                "strafe"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (!Contains(
                path,
                "idle") &&
            !Contains(
                path,
                "stay"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "crouch_stay"))
        {
            score +=
                2500;
        }

        if (Contains(
                path,
                "idle"))
        {
            score +=
                1800;
        }

        if (Contains(
                path,
                "stay"))
        {
            score +=
                900;
        }

        return
            score;
    }

    int ScoreCrouchMove(
        const std::string& path)
    {
        if (!Contains(
                path,
                "crouch"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path) ||
            IsLocomotionTransition(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "_to_") ||
            Contains(
                path,
                "crawl") ||
            Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "death") ||
            Contains(
                path,
                "sprint") ||
            Contains(
                path,
                "swim") ||
            Contains(
                path,
                "ladder"))
        {
            return
                std::numeric_limits<int>::min();
        }

        const bool movement =
            Contains(
                path,
                "walk") ||
            Contains(
                path,
                "run") ||
            Contains(
                path,
                "forward");

        if (!movement)
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "walk_forwardr"))
        {
            score +=
                3300;
        }
        else if (Contains(
                     path,
                     "walk_forward"))
        {
            score +=
                3100;
        }
        else if (Contains(
                     path,
                     "run_forwardr"))
        {
            score +=
                2600;
        }
        else if (Contains(
                     path,
                     "run_forward"))
        {
            score +=
                2400;
        }
        else if (Contains(
                     path,
                     "forward"))
        {
            score +=
                1500;
        }

        if (Contains(
                path,
                "walk"))
        {
            score +=
                1000;
        }

        if (Contains(
                path,
                "stay_unarmed"))
        {
            score +=
                700;
        }

        if (Contains(
                path,
                "back"))
        {
            score -=
                1200;
        }

        if (Contains(
                path,
                "strafe"))
        {
            score -=
                1200;
        }

        if (Contains(
                path,
                "left") ||
            Contains(
                path,
                "right"))
        {
            score -=
                800;
        }

        return
            score;
    }

    int ScoreJump(
        const std::string& path)
    {
        if (!Contains(
                path,
                "jump"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "down") ||
            Contains(
                path,
                "land"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "jump_up"))
        {
            score +=
                1200;
        }

        if (Contains(
                path,
                "run_forward_jump_up__unarmed"))
        {
            score +=
                1800;
        }

        if (Contains(
                path,
                "forward"))
        {
            score +=
                300;
        }

        if (Contains(
                path,
                "fly"))
        {
            score -=
                800;
        }

        return score;
    }

    int ScoreFall(
        const std::string& path)
    {
        if (!Contains(
                path,
                "fall") &&
            !Contains(
                path,
                "jump_fly"))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (IsInjuredVariant(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        if (Contains(
                path,
                "down") ||
            Contains(
                path,
                "land"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "run_forward_jump_fly__unarmed"))
        {
            score +=
                1800;
        }

        if (Contains(
                path,
                "jump_fly"))
        {
            score +=
                1200;
        }

        if (Contains(
                path,
                "forward"))
        {
            score +=
                300;
        }

        return score;
    }

    int ScoreLand(
        const std::string& path)
    {
        if (IsInjuredVariant(
                path))
        {
            return
                std::numeric_limits<int>::min();
        }

        const bool landingName =
            Contains(
                path,
                "land") ||
            Contains(
                path,
                "jump_down") ||
            Contains(
                path,
                "falldwn");

        if (!landingName)
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "jump_falldwnstrong"))
        {
            score +=
                1800;
        }

        if (Contains(
                path,
                "down_stop"))
        {
            score +=
                900;
        }

        if (Contains(
                path,
                "down_move"))
        {
            score +=
                600;
        }

        if (Contains(
                path,
                "land"))
        {
            score +=
                700;
        }

        return score;
    }

    int Score(
        const client::character::AnimationState state,
        const std::string& path)
    {
        using client::character::AnimationState;

        switch (state)
        {
        case AnimationState::Idle:
            return
                ScoreIdle(
                    path);

        case AnimationState::WalkForward:
        case AnimationState::WalkBackward:
        case AnimationState::WalkStrafeLeft:
        case AnimationState::WalkStrafeRight:
            return
                ScoreWalk(
                    path);

        case AnimationState::RunForward:
        case AnimationState::RunBackward:
        case AnimationState::RunStrafeLeft:
        case AnimationState::RunStrafeRight:
            return
                ScoreRun(
                    path);

        case AnimationState::Sprint:
            return
                ScoreSprint(
                    path);

        case AnimationState::CrouchIdle:
            return
                ScoreCrouchIdle(
                    path);

        case AnimationState::CrouchForward:
        case AnimationState::CrouchBackward:
        case AnimationState::CrouchStrafeLeft:
        case AnimationState::CrouchStrafeRight:
            return
                ScoreCrouchMove(
                    path);

        case AnimationState::TurnLeft:
        case AnimationState::TurnRight:
            return
                ScoreIdle(
                    path);

        case AnimationState::CrouchTurnLeft:
        case AnimationState::CrouchTurnRight:
            return
                ScoreCrouchIdle(
                    path);

        case AnimationState::Jump:
            return
                ScoreJump(
                    path);

        case AnimationState::Fall:
            return
                ScoreFall(
                    path);

        case AnimationState::Land:
            return
                ScoreLand(
                    path);

        case AnimationState::Count:
            break;
        }

        return
            std::numeric_limits<int>::min();
    }
}

namespace client::character
{
    std::string_view AnimationSet::Path(
        const AnimationState state) const noexcept
    {
        return
            paths[
                AnimationStateIndex(
                    state)];
    }

    bool AnimationCatalog::Resolve(
        const core::resources::ResourceFileSystem& resources,
        AnimationSet& output,
        std::string& error) const
    {
        output =
            {};

        error.clear();

        std::vector<
            const core::resources::ResourceEntry*>
            animations =
                resources.FindByType(
                    core::resources::ResourceType::Animation);

        std::sort(
            animations.begin(),
            animations.end(),
            [](
                const core::resources::ResourceEntry* left,
                const core::resources::ResourceEntry* right)
            {
                if (left ==
                    nullptr)
                {
                    return
                        right !=
                        nullptr;
                }

                if (right ==
                    nullptr)
                {
                    return false;
                }

                return
                    left->logicalPath <
                    right->logicalPath;
            });

        if (resources.Exists(
                VerifiedIdlePath))
        {
            output.paths[
                AnimationStateIndex(
                    AnimationState::Idle)] =
                VerifiedIdlePath;
        }

        for (std::size_t stateIndex = 0;
             stateIndex <
                 AnimationStateCount;
             ++stateIndex)
        {
            const AnimationState state =
                static_cast<AnimationState>(
                    stateIndex);

            if (!output.paths[
                    stateIndex].
                    empty())
            {
                continue;
            }

            const core::resources::ResourceEntry*
                selected =
                    nullptr;

            int selectedScore =
                std::numeric_limits<int>::min();

            for (const core::resources::ResourceEntry*
                 entry :
                 animations)
            {
                if (entry ==
                    nullptr)
                {
                    continue;
                }

                const std::string path =
                    Lower(
                        entry->logicalPath);

                const int score =
                    Score(
                        state,
                        path);

                if (score >
                    selectedScore)
                {
                    selected =
                        entry;

                    selectedScore =
                        score;
                }
            }

            if (selected ==
                    nullptr ||
                selectedScore ==
                    std::numeric_limits<int>::min())
            {
                error =
                    "Unable to find real animation resource for state " +
                    std::string(
                        AnimationStateName(
                            state)) +
                    ".";

                return false;
            }

            output.paths[
                stateIndex] =
                selected->logicalPath;
        }

        for (std::size_t stateIndex = 0;
             stateIndex <
                 AnimationStateCount;
             ++stateIndex)
        {
            const AnimationState state =
                static_cast<AnimationState>(
                    stateIndex);

            core::Log::Info(
                std::string(
                    "Character animation [") +
                std::string(
                    AnimationStateName(
                        state)) +
                "]: " +
                output.paths[
                    stateIndex]);
        }

        return true;
    }
}