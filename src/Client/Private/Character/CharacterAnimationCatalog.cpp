#include "Character/CharacterAnimationCatalog.h"

#include "Core/Log.h"
#include "Core/Resources/ResourceType.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
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
                300;
        }

        if (Contains(
                path,
                "/animations/"))
        {
            score +=
                150;
        }

        if (Contains(
                path,
                "unarmed"))
        {
            score +=
                250;
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
                "melee"))
        {
            score -=
                500;
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

        if (Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "death"))
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
                250;
        }

        if (Contains(
                path,
                "move_stay"))
        {
            score +=
                300;
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
                "death"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "forward"))
        {
            score +=
                300;
        }

        if (Contains(
                path,
                "back"))
        {
            score -=
                150;
        }

        if (Contains(
                path,
                "left") ||
            Contains(
                path,
                "right"))
        {
            score -=
                100;
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

        if (Contains(
                path,
                "jump") ||
            Contains(
                path,
                "fall") ||
            Contains(
                path,
                "death"))
        {
            return
                std::numeric_limits<int>::min();
        }

        int score =
            CommonScore(
                path);

        if (Contains(
                path,
                "forward"))
        {
            score +=
                300;
        }

        if (Contains(
                path,
                "back"))
        {
            score -=
                150;
        }

        if (Contains(
                path,
                "left") ||
            Contains(
                path,
                "right"))
        {
            score -=
                100;
        }

        if (Contains(
                path,
                "sprint"))
        {
            score -=
                50;
        }

        return score;
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
                "run_forward_jump_up__unarmed_old"))
        {
            score +=
                2000;
        }

        if (Contains(
                path,
                "jump_up"))
        {
            score +=
                700;
        }

        if (Contains(
                path,
                "forward"))
        {
            score +=
                200;
        }

        if (Contains(
                path,
                "fly"))
        {
            score -=
                500;
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
                "run_forward_jump_fly__unarmed_old"))
        {
            score +=
                2000;
        }

        if (Contains(
                path,
                "jump_fly"))
        {
            score +=
                800;
        }

        if (Contains(
                path,
                "forward"))
        {
            score +=
                150;
        }

        return score;
    }

    int ScoreLand(
        const std::string& path)
    {
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
                1500;
        }

        if (Contains(
                path,
                "down_stop"))
        {
            score +=
                700;
        }

        if (Contains(
                path,
                "down_move"))
        {
            score +=
                500;
        }

        if (Contains(
                path,
                "land"))
        {
            score +=
                600;
        }

        return score;
    }

    int Score(
        const client::character::AnimationState state,
        const std::string& path)
    {
        switch (state)
        {
            case client::character::AnimationState::Idle:
                return
                    ScoreIdle(
                        path);

            case client::character::AnimationState::Walk:
                return
                    ScoreWalk(
                        path);

            case client::character::AnimationState::Run:
                return
                    ScoreRun(
                        path);

            case client::character::AnimationState::Jump:
                return
                    ScoreJump(
                        path);

            case client::character::AnimationState::Fall:
                return
                    ScoreFall(
                        path);

            case client::character::AnimationState::Land:
                return
                    ScoreLand(
                        path);

            case client::character::AnimationState::Count:
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
                if (left == nullptr)
                {
                    return
                        right != nullptr;
                }

                if (right == nullptr)
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