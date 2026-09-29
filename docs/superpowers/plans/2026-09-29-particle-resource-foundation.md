# Particle Resource Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete Stage 1 of particle integration so every particle action and renderer format present in `packs/res/particles` can be resolved, parsed, simulated, and rendered from existing direct map placements.

**Architecture:** Core owns canonical effect paths, parsed particle definitions, and renderer-independent simulation. Client preview code resolves textures and visual geometry into shared scene assets; sprite-family particles use `ParticleRenderer`, while mesh-family particles reuse the main renderer's immutable mesh buffers with per-particle instances.

**Tech Stack:** C++20, Direct3D 11, HLSL Shader Model 5, MSBuild `.vcxproj`, existing `ResourceFileSystem`, `PackedSectionReader`, visual/mesh/material loaders.

**Spec:** `docs/superpowers/specs/2026-09-29-complete-particle-integration-design.md`

## Global Constraints

- Stage 1 covers direct chunk particle placements and all formats in `packs/res/particles`; `ParticleUDO`, `SFX_UDO`, and `ZoneEffectsUDO` remain mandatory later stages.
- Final x64 and Release x64 must consume the same Core, preview, runtime, and rendering implementation.
- Do not rewrite any packed resource file or hard-code map/particle allow-lists.
- Missing optional particle dependencies disable only the affected emitter and produce a resource-scoped warning.
- Do not add third-party dependencies.
- At the user's request, do not add or run automated tests and do not build either configuration. Verification in this plan is limited to static source/diff inspection; the user owns compilation and runtime testing.

## Review Focus

- Mixed case, backslashes, already-rooted paths, and omitted `.xml` must all resolve to the same canonical resource.
- A missing texture or visual must not abort unrelated map geometry or particle emitters.
- Every known action/renderer name in the current pack must leave `Unsupported` counts at zero when the user later runs the explicit audit.
- Empty particle sets, zero capacity, and capacity multiplication for amp/blur quads must not under-allocate the dynamic vertex buffer.
- Mesh/visual particle geometry must be cached as immutable scene meshes while transforms, size, rotation, and tint remain per live particle.

---

### Task 1: Canonical Effect Resource Resolver

**Files:**
- Create: `src/Core/Public/Core/World/Effects/EffectResourceResolver.h`
- Create: `src/Core/Private/World/Effects/EffectResourceResolver.cpp`
- Modify: `src/Core/Core.vcxproj`
- Modify: `src/Core/Private/World/WorldLoader.cpp`
- Modify: `src/Core/Private/World/Particles/ParticleLoader.cpp`
- Modify: `src/Core/Private/World/Particles/ParticlePackAudit.cpp`

**Interfaces:**
- Produces: `enum class EffectResourceKind : std::uint8_t { Particle, Sfx }`.
- Produces: `static std::string EffectResourceResolver::Resolve(const resources::ResourceFileSystem&, std::string_view, EffectResourceKind)`.
- Resolution order: normalized existing rooted path; `res/<reference>`; `sys/<reference>`; kind directory (`res/particles/` or `res/sfx/`) plus reference. Append `.xml` only when the normalized reference has no extension.

- [ ] **Step 1: Add the focused resolver interface and implementation**

Reject empty, absolute, parent-traversal, and non-XML references. Return the canonical `ResourceEntry::logicalPath` from `ResourceFileSystem::Find`, not a reconstructed spelling.

- [ ] **Step 2: Route all current particle callers through the resolver**

Delete `WorldLoader.cpp::BuildParticlePath` and `ParticleLoader.cpp::BuildResourcePath`; use `EffectResourceKind::Particle` in world loading, particle loading, and pack audit filtering.

- [ ] **Step 3: Register the new Core files**

Add one unconditional `ClInclude` and `ClCompile` entry so the Release Core library shared by both Client configurations contains the resolver.

- [ ] **Step 4: Perform static path-case inspection**

Use `rg` to confirm no private particle path builder remains and inspect the resolver branches for: `particles/foo`, `res/particles/foo.xml`, `sys/particles/foo`, `foo`, backslashes, uppercase extension, and invalid traversal. Do not execute a test binary.

- [ ] **Step 5: Commit**

Commit message: `Add canonical effect resource resolution`.

### Task 2: Complete Particle Action Definitions and Parsing

**Files:**
- Modify: `src/Core/Public/Core/World/Particles/ParticleActionDefinition.h`
- Modify: `src/Core/Private/World/Particles/ParticleLoader.cpp`
- Modify: `src/Core/Private/World/Particles/ParticlePackAudit.cpp`

**Interfaces:**
- Produces: `ParticleActionType::MatrixSwarm`, `ParticleActionType::NodeClamp`, and `ParticleActionType::Splat`.
- Produces: `ParticleMatrixSwarmAction { ParticleActionCommon common; }`.
- Produces: `ParticleNodeClampAction { ParticleActionCommon common; bool fullyClamp = true; }`.
- Produces: `ParticleSplatAction { ParticleActionCommon common; }`.
- Extends: `ParticleActionData` with all three concrete structs.

- [ ] **Step 1: Extend the action model**

Place the new enum values before `Unsupported`, preserve distinct concrete variant types, and keep shared delay/minimum-age parsing in `ParticleActionCommon`.

- [ ] **Step 2: Parse the three packed action sections**

`MatrixSwarm` and `Splat` read common fields. `NodeClamp` additionally reads optional `fullyClamp_`. A malformed known field returns an action-specific error; unknown action names retain the existing `Unsupported` path.

- [ ] **Step 3: Update audit classification**

Known new actions must no longer increment `unsupportedActionCount`; the audit continues to report genuinely unknown names.

- [ ] **Step 4: Statically compare known action names with the pack inventory**

Use binary-safe `rg -a`/string inventory and a source-name scan to confirm the loader recognizes `Source`, `Sink`, `TintShader`, `Orbitor`, `Jitter`, `Stream`, `Force`, `Magnet`, `Barrier`, `Scaler`, `Flare`, `Collide`, `MatrixSwarm`, `NodeClamp`, and `Splat`.

- [ ] **Step 5: Commit**

Commit message: `Parse all packaged particle actions`.

### Task 3: Complete Particle Renderer Definitions and Parsing

**Files:**
- Modify: `src/Core/Public/Core/World/Particles/ParticleRendererDefinition.h`
- Modify: `src/Core/Private/World/Particles/ParticleLoader.cpp`
- Modify: `src/Core/Private/World/Particles/ParticlePackAudit.cpp`

**Interfaces:**
- Extends: `ParticleRendererType` with `PointSprite`, `Blur`, `Amp`, `Mesh`, and `Visual` before `Unsupported`.
- Extends: `ParticleRendererDefinition` with `visualName`, `height`, `time`, `variation`, `circular`, `doubleSided`, and `sortType`; reuse existing `texture`, `width`, `steps`, and `materialFx` fields.
- Keeps: texture and animated-texture resolution through `ParticleTextureReference` for every texture-bearing renderer.

- [ ] **Step 1: Extend the renderer model without renderer-specific inheritance**

Keep the existing value-object pattern. Defaults must be safe for missing optional fields and preserve existing Sprite/SpriteBlend/Trail behavior.

- [ ] **Step 2: Map every packaged renderer section to a known enum**

Recognize `PointSpriteParticleRenderer`, `BlurParticleRenderer`, `AmpParticleRenderer`, `MeshParticleRenderer`, and `VisualParticleRenderer` in addition to the three existing renderer names.

- [ ] **Step 3: Parse renderer-specific fields and dependencies**

Parse Amp `textureName_`, `width_`, `height_`, `steps_`, `variation_`, and `circular_`; Blur `textureName_`, `width_`, and `time_`; PointSprite using sprite-compatible fields; Mesh `visualName_`, `textureName_`, `sortType_`, `materialFX_`, and `doubleSided_`; Visual `visualName_`. Resolve texture references with the existing texture resolver and normalize visual references for later loading.

- [ ] **Step 4: Statically compare renderer names with the pack inventory**

Confirm the loader recognizes Sprite, SpriteBlend, Trail, PointSprite, Blur, Amp, Mesh, and Visual and that only unknown future names enter the `Unsupported` audit branch.

- [ ] **Step 5: Commit**

Commit message: `Parse all packaged particle renderers`.

### Task 4: Runtime Support for MatrixSwarm, NodeClamp, and Splat

**Files:**
- Create: `src/Core/Public/Core/World/Particles/ParticleCollisionQuery.h`
- Modify: `src/Core/Public/Core/World/Particles/ParticleRuntime.h`
- Modify: `src/Core/Private/World/Particles/ParticleRuntime.cpp`
- Modify: `src/Core/Core.vcxproj`
- Modify: `src/Client/Private/World/WorldCollision.h`
- Modify: `src/Client/Private/World/WorldCollision.cpp`
- Modify: `src/Client/Private/Graphics/Renderer.h`
- Modify: `src/Client/Private/Graphics/Renderer.cpp`
- Modify: `src/Client/Private/World/WorldSession.cpp`
- Modify: `src/Client/Private/Studio/Application.h`
- Modify: `src/Client/Private/Studio/Application.cpp`

**Interfaces:**
- Produces: abstract `ParticleCollisionQuery::Raycast(start, end, fraction, normal) const noexcept -> bool`.
- Produces: `ParticleRuntimeSystem::SetTransform(const math::Transform3x4&) noexcept`.
- Produces: `ParticleRuntimeSystem::SetSwarmTargets(std::span<const math::Transform3x4>)` that copies target transforms into instance state.
- Produces: `ParticleRuntimeSystem::SetCollisionQuery(const ParticleCollisionQuery*) noexcept` with non-owning scene-lifetime semantics.
- Extends: `client::world::Collision` to implement `ParticleCollisionQuery` and return the hit normal as well as fraction.
- Extends: `Renderer::SetScene` with a non-owning `const ParticleCollisionQuery*`; Final passes `WorldSession::collision_`, while Release Studio owns and passes its own `client::world::Collision` built from the same `SceneRenderData`.

- [ ] **Step 1: Add the Core collision boundary and runtime inputs**

Core must not include Client headers. Initialize previous-emitter position from the initial transform, clear swarm targets on initialization, and retain only a non-owning collision pointer.

- [ ] **Step 2: Implement NodeClamp behavior**

After its common delay/minimum-age gate, fully-clamped particles move to the emitter origin; relative mode adds the emitter's frame-to-frame displacement. Update the remembered emitter position once per runtime frame.

- [ ] **Step 3: Implement MatrixSwarm behavior**

After its common gate, distribute live particles deterministically across copied target transforms. With no targets, perform no override.

- [ ] **Step 4: Implement Splat behavior**

After particle integration, raycast from `previousPosition` to `position`; remove the particle on a hit. With no collision query, perform no collision and keep the particle alive.

- [ ] **Step 5: Connect a scene-lifetime collision object in both applications**

Extend `Renderer::SetScene` to accept the non-owning collision query and assign it to every particle runtime system before frame updates. Final passes the collision already built by `WorldSession`; Release Studio adds a `Collision` member, builds it from `scene_`, and passes it through the same API. Declare and clear members so the query outlives renderer use. Preserve current collide/barrier statistics and add a distinct `splatInteractions` counter.

- [ ] **Step 6: Perform static lifetime and switch coverage review**

Inspect all `ParticleActionType` switches for explicit handling or intentional no-op behavior. Confirm the collision provider outlives particle systems in both Final and Release, every `SetScene` caller supplies the appropriate query, and no Core file depends on Client.

- [ ] **Step 7: Commit**

Commit message: `Simulate remaining particle actions`.

### Task 5: Particle Definition Cache and Reachability-Based Loading

**Files:**
- Create: `src/Core/Public/Core/World/Particles/ParticleDefinitionCache.h`
- Create: `src/Core/Private/World/Particles/ParticleDefinitionCache.cpp`
- Modify: `src/Core/Core.vcxproj`
- Modify: `src/Client/Private/Preview/WorldPreviewLoader.cpp`
- Modify: `src/Client/Private/Preview/ParticleRuntimeDataBuilder.h`
- Modify: `src/Client/Private/Preview/ParticleRuntimeDataBuilder.cpp`

**Interfaces:**
- Produces: `ParticleDefinitionCache::Load(const ResourceFileSystem&, std::string_view, const ParticleDefinition*&, std::string&) -> bool`.
- Produces: `ParticleDefinitionCache::Definitions() const noexcept -> const std::unordered_map<std::string, ParticleDefinition>&`.
- Produces: `ParticleDefinitionCache::Clear() noexcept` and `Size() const noexcept`.

- [ ] **Step 1: Implement canonical-path definition caching**

Resolve first, parse once per canonical logical path, and return a stable pointer owned by the cache. Failed loads are returned to the caller but not inserted as successful definitions.

- [ ] **Step 2: Replace local manual loading in `LoadWorldPreview`**

Load only particle paths reachable from `world.particleInstances`. Keep per-resource warnings and missing dependency statistics, but remove duplicated definition bookkeeping.

- [ ] **Step 3: Remove normal-load full-pack auditing**

Delete the `ParticlePackAuditor::Run` block from `WorldPreviewLoader.cpp`. Keep the auditor class and its explicit diagnostic behavior available for later user-initiated validation.

- [ ] **Step 4: Adapt runtime-data building to cached definitions**

Consume `ParticleDefinitionCache::Definitions()` without copying definitions a second time; preserve one emitter per particle subsystem and the current instance transform.

- [ ] **Step 5: Perform static reachability review**

Confirm no normal world-load call scans `FindByType(ResourceType::Xml)` and every direct placement still reaches `ParticleRuntimeDataBuilder`.

- [ ] **Step 6: Commit**

Commit message: `Cache reachable particle definitions`.

### Task 6: Particle Visual and Mesh Render Data

**Files:**
- Create: `src/Client/Private/Preview/ParticleMeshRenderDataBuilder.h`
- Create: `src/Client/Private/Preview/ParticleMeshRenderDataBuilder.cpp`
- Modify: `src/Client/Client.vcxproj`
- Modify: `src/Client/Private/Graphics/SceneRenderData.h`
- Modify: `src/Client/Private/Preview/ParticleRenderDataBuilder.h`
- Modify: `src/Client/Private/Preview/ParticleRenderDataBuilder.cpp`
- Modify: `src/Client/Private/Preview/WorldPreviewLoader.cpp`

**Interfaces:**
- Extends: `SceneParticleEmitter` with `std::vector<std::size_t> meshIndices` and renderer-ready material metadata.
- Produces: `ParticleMeshRenderDataBuilder::Build(const ResourceFileSystem&, const ParticleRendererDefinition&, SceneRenderData&, std::vector<std::size_t>&, std::string&) -> bool`.
- Reuses: `VisualLoader`, `MeshLoader`, and `ModelRenderDataBuilder` to append immutable `SceneMesh` entries and their textures/materials.

- [ ] **Step 1: Add emitter references to shared scene meshes**

Do not copy `MeshData` into each emitter. Empty mesh-index vectors are valid for sprite-family renderers.

- [ ] **Step 2: Load and cache mesh/visual particle dependencies**

Resolve each renderer `visualName`, load every visual geometry, build its model materials, append it once to `SceneRenderData::meshes`, and cache the resulting index vector by canonical visual path.

- [ ] **Step 3: Generalize texture preparation for all texture-bearing renderers**

PointSprite, Blur, Amp, Mesh texture overrides, Sprite, SpriteBlend, and Trail all use the existing static/animated texture cache. A missing texture or visual disables only that emitter and logs its particle resource/system context.

- [ ] **Step 4: Keep world loading tolerant of local render dependency failures**

Mark failed emitters non-renderable instead of returning a fatal `LoadWorldPreview` error. Preserve runtime simulation for other emitters.

- [ ] **Step 5: Perform static cache/reference review**

Confirm repeated visual paths append one mesh set, mesh indices remain valid after vector growth, and particle-local state contains no owning duplicate geometry.

- [ ] **Step 6: Commit**

Commit message: `Prepare mesh particle render resources`.

### Task 7: PointSprite, Blur, and Amp GPU Geometry

**Files:**
- Modify: `src/Client/Private/Graphics/ParticleRenderer.cpp`
- Modify if required: `game/Shaders/World/WorldParticle.hlsl`
- Modify if required: `game/Studio/Shaders/World/WorldParticle.hlsl`

**Interfaces:**
- Keeps: the existing `ParticleRenderer::Render(...)` public signature.
- Adds internally: geometry appenders for point sprite, blur segment, and amp strip commands.
- Extends internally: vertex-capacity calculation to account for renderer-specific worst-case vertices.

- [ ] **Step 1: Render PointSprite through camera-facing quad expansion**

Use the existing sprite size, rotation, colour, texture animation, blend, fog, and depth-read semantics; keep a distinct renderer enum for diagnostics.

- [ ] **Step 2: Render Blur as a motion-oriented textured quad**

Construct the segment from current position toward `-velocity * time`, falling back to `previousPosition`; use configured width and particle tint. Skip only degenerate zero-length segments.

- [ ] **Step 3: Render Amp as a connected textured strip**

Order live particles by container order, connect consecutive points, close the strip when `circular` is true, subdivide by `steps`, apply deterministic variation from system/segment identity, and use width/height without frame-to-frame random flicker.

- [ ] **Step 4: Make buffer sizing renderer-aware**

Compute the maximum generated vertices for sprite/point-sprite, trail/blur, and amp strip geometry using checked `size_t` arithmetic. Return a resource-scoped error before allocation overflow.

- [ ] **Step 5: Keep shader copies synchronized if shader input changes**

If no new shader input is necessary, leave both files untouched. Otherwise apply identical changes and verify with `Compare-Object`/hash comparison only.

- [ ] **Step 6: Statically inspect all renderer dispatch branches**

Confirm Sprite, SpriteBlend, Trail, PointSprite, Blur, and Amp create draw commands; Mesh and Visual are intentionally delegated to Task 8; Unsupported and None are skipped explicitly.

- [ ] **Step 7: Commit**

Commit message: `Render remaining sprite particle types`.

### Task 8: Mesh and Visual Particle Instances

**Files:**
- Modify: `src/Client/Private/Graphics/SceneRenderData.h`
- Modify: `src/Client/Private/Graphics/Renderer.cpp`
- Modify: `src/Core/Public/Core/World/Particles/ParticleRuntime.h`

**Interfaces:**
- Produces internally: one dynamic render instance per live mesh/visual particle and per referenced `meshIndex`.
- Uses: `ParticleRuntimeParticle` position, rotation, angular state, size, and colour plus the emitter transform rules already applied by the runtime.
- Uses: immutable GPU meshes uploaded by the existing scene mesh upload path.

- [ ] **Step 1: Add per-instance particle tint to the internal render-instance path**

Default world/model instances retain white tint. Particle mesh instances carry normalized runtime colour into the existing model constants without mutating shared material data.

- [ ] **Step 2: Generate mesh/visual particle transforms each frame**

Compose translation, rotation/spin, uniform size, and any remaining local renderer transform. Mesh and Visual share the scene-mesh path; Mesh applies its materialFX/sort/double-sided metadata, while Visual keeps source material behavior.

- [ ] **Step 3: Append dynamic instances without persisting stale particles**

Rebuild particle-derived instances after runtime update and before opaque/transparent render-list classification. Do not append them to the immutable base instance list.

- [ ] **Step 4: Preserve blend, depth, culling, and ordering semantics**

Map particle material modes onto existing opaque/cutout/blend/additive states. Apply `doubleSided` through the appropriate rasterizer state and sort accurate mesh particles with transparent instances.

- [ ] **Step 5: Perform static mesh-index and state-restoration review**

Check all bounds before indexing GPU meshes/materials, confirm empty/failed mesh emitters are skipped locally, and ensure rasterizer/blend/depth state is restored before the following render pass.

- [ ] **Step 6: Commit**

Commit message: `Render mesh and visual particles`.

### Task 9: Stage 1 Static Integration Review

**Files:**
- Review: all files changed by Tasks 1-8
- Review: `src/Core/Core.vcxproj`
- Review: `src/Client/Client.vcxproj`
- Review: `game/Shaders/World/WorldParticle.hlsl`
- Review: `game/Studio/Shaders/World/WorldParticle.hlsl`

**Interfaces:**
- Consumes: every Stage 1 interface above.
- Produces: a source-only handoff for the user's Release and Final builds.

- [ ] **Step 1: Scan known packaged types against loader/runtime/renderer dispatch**

Use binary-safe resource string inventory plus `rg` to ensure the three added actions and five added renderers are parsed and reach a runtime or renderer branch.

- [ ] **Step 2: Inspect failure locality**

Trace missing particle XML, texture animation, DDS, visual, and primitives paths. Confirm each disables/logs only its resource or emitter and does not abort unrelated world geometry.

- [ ] **Step 3: Inspect shared configuration coverage**

Confirm new Core/Client source files are unconditional project entries and no implementation is hidden behind only `Final|x64` or only `Release|x64` conditions.

- [ ] **Step 4: Run non-executing repository checks only**

Run `git diff --check`, inspect `git status --short`, compare shader copies when changed, and review the complete diff. Do not invoke MSBuild, the client/editor executable, a test runner, or a generated diagnostic binary.

- [ ] **Step 5: Commit any review-only corrections**

Commit message: `Harden particle resource foundation` only if corrections were required; otherwise leave the task without an empty commit.
