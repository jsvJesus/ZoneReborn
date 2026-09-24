# Character Appearance Creator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (- [ ]) syntax for tracking.

**Goal:** Add a head-focused appearance submenu whose controls drive the real character models, facial materials, facial bones, and persisted character profile.

**Architecture:** Clothing remains in the current creator catalog. A separate face catalog is owned by character::Catalog; native FaceState is authoritative; style changes use slots 130-132; facial transforms are applied before CPU skinning; facial colours and overlays travel in an optional material payload. WebView renders controls and sends semantic choices, while C++ validates, randomizes, snapshots, renders, and saves.

**Tech Stack:** C++20, Direct3D 11/HLSL 5.0, WebView2, vanilla HTML/CSS/JavaScript, MSBuild/Visual Studio 2022.

**Spec:** docs/superpowers/specs/2026-09-24-character-appearance-design.md

## Global Constraints

- Preserve the accepted frontend font, top account/balance/language bar, red markers, translucent panels, spacing, and responsive behavior.
- charMakerCfg.json stays clothing-only. Use charMakerFaceCfg.json for facial choices and CLOTH.pyson for face-part models.
- Opening/editing appearance never writes user/characters/*.dat.
- Direct mouse sculpting and scroll zoom are excluded. Face form is randomized with original FaceSaver limits.
- Exclude donor-only face choices.
- Load CHARACTER_V1; write CHARACTER_V2.
- Add no Python runtime or third-party dependency.
- Add no automated tests, per the user's request. Use syntax, build, diagnostic, and runtime verification.
- Never stage or modify the user's deletion of user/characters/test.dat.

## File Structure

- src/Client/Private/Character/CharacterFaceState.h: shared face state.
- src/Client/Private/Character/CharacterFaceCatalog.h/.cpp: face JSON parsing, validation, weights.
- src/Client/Private/Character/CharacterFaceCodec.h/.cpp: FaceSettings/FaceSaver limits, packing, decoding, random generation.
- CharacterCatalog.*: owns the face catalog and facial item material metadata.
- CharacterState.*: applies/reset/randomizes semantic face controls.
- CharacterProfile.h and CharacterService.*: CHARACTER_V2 persistence.
- CharacterAnimator.*: applies face transforms during animation/skinning.
- CharacterRenderDataBuilder.* and Graphics/*: facial material data and rendering.
- WorldShared.hlsli and WorldPixel.hlsl: facial shader path.
- Frontend.* and Application.*: commands, snapshots, camera, integration.
- packs/frontend/characters/appearance.js/.css: appearance controls in the accepted design.
- create.html/.js/.css, core/frontend.js, index.html: host and route appearance mode.
- Client.vcxproj and Client.vcxproj.filters: new native files.

## Review Focus

- Malformed, oversized, or truncated packed face forms fall back/reject without out-of-range reads: Task 3 diagnostics.
- Style 0 clears only its matching face slot and preserves clothes: Task 4 runtime inspection.
- Rapid scalar/colour input performs material updates, not repeated model reloads: Task 9 runtime logs.
- Apply, back, creator exit, and failed create restore the correct snapshot/camera: Tasks 6-7.
- V1 defaulting, V2 round-trip, and invalid V2 data are deterministic: Task 5 disposable-profile checks.

---

### Task 1: Checkpoint the Existing First-Stage Creator

**Files:**
- Existing modified frontend creator files.
- Existing modified Application, CharacterService, and Frontend files.

**Interfaces:**
- Consumes: current main menu and character renderer.
- Produces: stable character-create baseline for this stage.

- [ ] **Step 1: Re-run frontend syntax checks**

~~~powershell
node --check packs/frontend/core/frontend.js
node --check packs/frontend/main/main.js
node --check packs/frontend/characters/create.js
~~~

Expected: all exit 0.

- [ ] **Step 2: Rebuild Final x64**

~~~powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' src\ZoneReborn.sln /m /t:Build /p:Configuration=Final /p:Platform=x64 /v:minimal
~~~

Expected: Build succeeded and game/ZoneReborn.exe updated.

- [ ] **Step 3: Commit only first-stage files**

Stage explicit frontend/Application/CharacterService/Frontend paths. Confirm user/characters/test.dat is absent from git diff --cached. Commit as feat: add first-stage character creator.

### Task 2: Add the Face Domain Model and Catalog

**Files:**
- Create: src/Client/Private/Character/CharacterFaceState.h
- Create: src/Client/Private/Character/CharacterFaceCatalog.h
- Create: src/Client/Private/Character/CharacterFaceCatalog.cpp
- Modify: CharacterCatalog.h/.cpp, CharacterState.h/.cpp
- Modify: Client.vcxproj and Client.vcxproj.filters

**Interfaces:**
- Produces FaceState; FaceCatalog::Load; IsStyleAllowed; IsColourAllowed; RandomWeighted; Catalog::Faces.
- FaceState holds hairStyle, moustacheStyle, beardStyle; existing byte/color/detail fields; vector<uint64_t> faceForm.

- [ ] **Step 1: Move FaceState into CharacterFaceState.h**

Use explicit fixed-width fields. Replace vector<int16_t> faceForm with vector<uint64_t>. Include this header from state/profile users.

- [ ] **Step 2: Define FaceCatalog**

~~~cpp
struct FaceWeightedValue final
{
    std::uint32_t value = 0;
    std::uint32_t weight = 100;
};

class FaceCatalog final
{
public:
    bool Load(const core::resources::ResourceFileSystem&, std::string& error);
    void Clear();
    bool IsStyleAllowed(std::string_view group, std::int32_t value) const noexcept;
    bool IsColourAllowed(std::string_view group, std::uint32_t value) const noexcept;
    std::uint32_t RandomWeighted(std::string_view group, std::mt19937&) const;
    const std::vector<std::int32_t>& Styles(std::string_view group) const noexcept;
};
~~~

- [ ] **Step 3: Parse charMakerFaceCfg.json**

Support PictureButtonList data item_id/weight/donat_only, ColorList rrggbb:weight entries, and the known slider IDs. Skip donor-only entries. Require non-empty normal groups. Permit style 0 only for hair/moustache/beard removal.

- [ ] **Step 4: Extend item metadata**

Add substrate, tintMaterial, lengthLimits, hasLengthLimits to ItemDefinition. Parse Substrate, first Tint value, and LengthLimits from CLOTH.pyson. Catalog::Load loads FaceCatalog after items/clothes; Catalog::Faces returns it.

- [ ] **Step 5: Add state mutation API**

~~~cpp
bool ResetFace(const Catalog&, std::string& error);
bool ApplyFaceValue(const Catalog&, std::string_view group,
                    std::uint64_t value, bool& modelChanged,
                    std::string& error);
bool ApplyFaceState(const Catalog&, const FaceState&, std::string& error);
bool RandomizeFace(const Catalog&, std::mt19937&, std::string& error);
~~~

Map percentages to bytes with rounded value*255/100. Map style groups to Hair/Moustache/Beard slots and validate every style/color.

- [ ] **Step 6: Register files, build, and commit**

Build Final x64. Confirm log reports loaded face groups. Commit as feat: load character face configuration.

### Task 3: Port FaceSaver and FaceSettings

**Files:**
- Create: CharacterFaceCodec.h/.cpp
- Modify: CharacterState.cpp
- Modify: Client.vcxproj and filters

**Interfaces:**
- Produces GenerateRandomFaceForm, DecodeFaceForm, ValidateFaceForm and named FaceBoneTransform values.

- [ ] **Step 1: Define codec interface**

~~~cpp
struct FaceBoneTransform final
{
    std::string bone;
    std::string pairedBone;
    std::array<float, 3> translation{};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
};

std::vector<std::uint64_t> GenerateRandomFaceForm(std::mt19937&);
bool DecodeFaceForm(std::span<const std::uint64_t>,
                    std::vector<FaceBoneTransform>&,
                    std::string& error);
bool ValidateFaceForm(std::span<const std::uint64_t>) noexcept;
~~~

- [ ] **Step 2: Port the exact handle table and codec**

Traverse unpaired FaceSettings handles in source order. Preserve sentinel bit, 8 bits per active value, little-endian uint64 word array, per-axis limits, identity inactive axes, and mirrored pair name.

- [ ] **Step 3: Port random generation**

Use random_face_factor 0.9 and RANDOM_CHARISMA 0.6. Narrow each range around its midpoint, average two uniform samples, then pack.

- [ ] **Step 4: Connect defaults/randomization**

ResetFace generates a valid default form. RandomizeFace chooses allowed weighted styles/colours, original age-biased scalar distributions, and a new packed form.

- [ ] **Step 5: Run diagnostic cases**

Through concise existing logging verify empty generation, valid decode handle count, rejection above 256 words, and rejection/defaulting of truncated sentinel data. Remove verbose temporary logs.

- [ ] **Step 6: Build and commit**

Commit as feat: port character face form codec.

### Task 4: Compose Hair, Moustache, and Beard Models

**Files:**
- Modify: CharacterState.cpp
- Modify: CharacterModelComposer.cpp
- Modify: CharacterRenderDataBuilder.cpp

**Interfaces:**
- Consumes FaceState style IDs.
- Produces actual CLOTH.pyson models through slots 130-132.

- [ ] **Step 1: Enforce style/slot invariants**

ApplyFaceValue updates both FaceState and one matching slot. ApplyFaceState clears all three face slots, then equips non-zero styles. On failure restore previous face state/slots.

- [ ] **Step 2: Preserve face slots during clothing operations**

ApplyCreatorSet changes only configured clothing groups. ResetCreator resets face and clothes together only at new-session reset.

- [ ] **Step 3: Verify model composition**

Cycle every allowed face style. Style 0 removes only that part; other styles load expected ManNude paths; clothes remain unchanged. Remove temporary model-path logging.

- [ ] **Step 4: Build and commit**

Commit as feat: compose character facial hair styles.

### Task 5: Persist CHARACTER_V2

**Files:**
- Modify: CharacterProfile.h
- Modify: CharacterService.h/.cpp
- Modify: Application.cpp
- Modify: Frontend.cpp

**Interfaces:**
- Profile gains FaceState face.
- Service::Create receives const FaceState& after clothing appearance.

- [ ] **Step 1: Add face state to profile/create flow**

Application passes characterState_.Face() only on final Create.

- [ ] **Step 2: Write exact V2 rows**

~~~text
face_style hair moustache beard
face_scalar hairLength beardLength moustacheLength age details unshaven eyebrowPosition eyebrowRotation
face_colour hairColor skinColor eyeColor tattooColor
face_detail eyebrowStyle tattooStyle
face_form_count N
face_form WORD
~~~

Repeat face_form for N words; cap N at 256.

- [ ] **Step 3: Load V1 and V2**

V1 stops after clothes and receives ResetFace defaults in memory. V2 requires all face rows, checks numeric bounds, validates packed form and allowed styles/colors.

- [ ] **Step 4: Apply profile atomically**

Build a temporary State, apply clothes then face, and assign only on total success.

- [ ] **Step 5: Verify disposable profiles**

Check V1 default load, V2 restart round-trip, truncated face row rejection, and count 257 rejection.

- [ ] **Step 6: Build and commit**

Commit as feat: persist character face state.

### Task 6: Add Bridge Events and Snapshots

**Files:**
- Modify: Frontend.h/.cpp
- Modify: Application.h/.cpp
- Modify: packs/frontend/core/frontend.js

**Interfaces:**
- Commands: character_face_open/value/random/reset/apply/cancel.
- Native callback: Frontend.receiveCharacterFaceState(object).

- [ ] **Step 1: Add six event enum values and payload fields**

Add faceChoiceGroup string and uint64 faceValue.

- [ ] **Step 2: Parse commands strictly**

Value requires exactly group and unsigned value. Reject missing/extra/overflow/signed input and unknown groups.

- [ ] **Step 3: Serialize authoritative FaceState**

Send slider percentages, RGB integers, style IDs, and face-form uint64 values as decimal strings to avoid JavaScript precision loss.

- [ ] **Step 4: Add Application snapshot lifecycle**

On open copy FaceState. Reset restores the snapshot. Random mutates current state. Apply discards snapshot. Cancel restores/rebuilds/discards. Creator exit clears snapshot.

- [ ] **Step 5: Add JavaScript bridge methods**

Expose openCharacterFace, setCharacterFaceValue, randomizeCharacterFace, resetCharacterFace, applyCharacterFace, cancelCharacterFace. Route native state to the active screen callback.

- [ ] **Step 6: Syntax/build/commit**

Run node --check and Final x64 build. Commit as feat: add character appearance bridge.

### Task 7: Add the Head Camera

**Files:**
- Modify: Application.h/.cpp

**Interfaces:**
- Produces BuildFaceCamera, SetCharacterCameraMode, UpdateCharacterCamera.

- [ ] **Step 1: Add FullBody/Face camera state**

Store current/from/to CameraView, blend start, mode, and active flag.

- [ ] **Step 2: Derive face view**

Target CharacterTransform origin plus Y=1.63, distance 0.63 along stage camera horizontal direction, upright Y, FOV 0.7 radians converted to degrees.

- [ ] **Step 3: Blend over 0.22 seconds**

Use smoothstep t*t*(3-2*t), normalize forward/up, and set current camera in Update instead of resetting stage camera every frame.

- [ ] **Step 4: Cover exits**

Apply/cancel/back/create/delete/shutdown/failure all restore FullBody and clear invalid blends.

- [ ] **Step 5: Verify and commit**

Check head framing, right-drag rotation, repeated open/back exact restoration at multiple sizes. Build and commit as feat: add character appearance camera.

### Task 8: Apply Face Form During CPU Skinning

**Files:**
- Modify: CharacterAnimator.h/.cpp
- Modify: CharacterRenderDataBuilder.cpp

**Interfaces:**
- Animator::SetFaceForm(span<const uint64_t>, error).

- [ ] **Step 1: Store decoded morph map**

Map bone names to FaceBoneTransform inside Animator::State.

- [ ] **Step 2: Pre-multiply local transforms**

After SampleChannel and before parent hierarchy multiplication, calculate local = Multiply(morph, local). Apply the same value to paired bones. Log missing bones once per rebuild.

- [ ] **Step 3: Set form before initial skinning**

RenderDataBuilder calls SetFaceForm after idle loading and before mesh update.

- [ ] **Step 4: Verify and commit**

Randomize repeatedly; observe facial regions; run idle one minute to ensure no accumulation; confirm symmetry. Build and commit as feat: render character face form.

### Task 9: Render Facial Materials

**Files:**
- Modify: Graphics/SceneRenderData.h, Renderer.h/.cpp
- Modify: Preview/ModelRenderDataBuilder.h/.cpp
- Modify: CharacterRenderDataBuilder.cpp
- Modify: WorldShared.hlsli and WorldPixel.hlsl

**Interfaces:**
- CharacterMaterialRole: None, Skin, Eyes, Hair, Moustache, Beard.
- Renderer::UpdateCharacterMaterials updates already-uploaded character material records.

- [ ] **Step 1: Add bounded material payload**

Add role, characterColour, two float4 parameter blocks, overlayTextureIndex, detailTextureIndex to SceneModelMaterial. Parameters0 is age/details/unshaven/lengthShift; parameters1 is brow angle/shift plus effect flags.

- [ ] **Step 2: Resolve roles/resources**

Pass optional character context into ModelRenderDataBuilder. Identify SkinTint, Eyes_skinned, hair_skinned, moustache_skinned, beard_skinned from model/material metadata. Load substrate, selected brow, selected battle-mark, and available normal/detail maps using the existing texture cache.

- [ ] **Step 3: Extend shader bindings**

Add matching constants. Reserve t11 overlay and t12 detail. Bind null after character draw. Leave role None on the existing path.

- [ ] **Step 4: Shade face roles**

Apply skin/eye/hair RGB; optional age/detail/unshaven blends; eyebrow/tattoo overlays; brow shift/angle; hair-part UV length shift. Preserve current cutout/blend behavior.

- [ ] **Step 5: Avoid slider rebuilds**

Application uses UpdateCharacterMaterials for scalar/color changes; RebuildCharacter only for styles and full random face form.

- [ ] **Step 6: Verify and commit**

Sweep every scalar/color rapidly and confirm no reload logs. Check all brow/tattoo choices and face-part lengths. Confirm world/background unaffected. Build and commit as feat: render character facial materials.

### Task 10: Build the Appearance UI

**Files:**
- Create: packs/frontend/characters/appearance.js
- Create: packs/frontend/characters/appearance.css
- Modify: create.html/.js/.css
- Modify: packs/frontend/index.html

**Interfaces:**
- window.CharacterAppearance.initialize/open/close/receiveState.

- [ ] **Step 1: Add hidden appearance layout**

Reuse the current top bar. Left categories: Face, Skin, Eyes, Hairstyle, Eyebrows, Mustache, Beard, Colour. Add Reset/Random/Apply/Back. Right panel hosts caption, icons, swatches, sliders, descriptions. Face category contains Random face form.

- [ ] **Step 2: Parse face config**

Flatten sorted groups, skip donor-only data, preserve item_id/weight/slider/RGB. Resolve original icons under /packs/res/soGUI/frame/character_creation/face/.

- [ ] **Step 3: Match accepted design**

Reuse main-* typography/dimensions/colours. Use dark translucent option tiles, red selected border/marker, square swatches, grey/red sliders, responsive panel sizing. Explicitly inherit font for controls.

- [ ] **Step 4: Bind live updates**

Clicks post immediately. Coalesce slider input to one latest value per requestAnimationFrame. Disable until authoritative initial state. Native state updates UI without echo.

- [ ] **Step 5: Bind semantics/localization**

Main appearance button opens native/UI mode. Apply retains; Back cancels; Reset/Random stay open. Add Russian/English/Chinese strings without disturbing header, balances, locale, version, or name draft.

- [ ] **Step 6: Check/visual review/commit**

Run node --check for core frontend, create.js, appearance.js. Review 1920x1080 and smaller supported size against accepted menu. Commit as feat: add character appearance submenu.

### Task 11: Integration and Completion Verification

**Files:**
- Only feature files from Tasks 2-10 as required by findings.

**Interfaces:**
- Produces shippable stage-two build.

- [ ] **Step 1: Static verification**

~~~powershell
node --check packs/frontend/core/frontend.js
node --check packs/frontend/main/main.js
node --check packs/frontend/characters/create.js
node --check packs/frontend/characters/appearance.js
git diff --check
~~~

- [ ] **Step 2: Full build**

~~~powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' src\ZoneReborn.sln /m /t:Build /p:Configuration=Final /p:Platform=x64 /v:minimal
~~~

- [ ] **Step 3: Manual end-to-end matrix**

Verify: open writes no profile; head camera; every control changes rendering; face random changes bones and controls; submenu reset/back/apply; creator reset/random; final create only on submit; V2 restart round-trip; delete unchanged.

- [ ] **Step 4: Failure/compatibility matrix**

Verify V1 default face, malformed command rejection, invalid style rejection, truncated V2 rejection, and optional overlay failure degrading to base rendering with warning.

- [ ] **Step 5: Inspect scope**

Run git status --short, git diff --stat, git diff --check. Keep user/characters/test.dat unstaged and generated outputs untracked/ignored.

- [ ] **Step 6: Final integration commit**

Commit only in-scope fixes as feat: complete character appearance creator.

- [ ] **Step 7: Evidence before completion**

Invoke superpowers:verification-before-completion, rerun commands supporting final claims, report game/ZoneReborn.exe, and distinguish automated build evidence from runtime checks that require user interaction.
