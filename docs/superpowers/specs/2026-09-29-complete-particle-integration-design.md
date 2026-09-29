# Complete Particle Integration Design

Date: 2026-09-29

## Goal

Make the shared game source resolve, load, simulate, and render every particle
effect reachable from the resource graph under `packs/res`, including effects
placed directly in a map and effects attached to objects, items, models, UDOs,
weather definitions, and SFX graphs. The same implementation must serve the
Final x64 client and the Release x64 editor and must work for existing and
future maps without per-map allow-lists.

## Constraints

- `packs/res/particles` and the resources referenced by those files are the
  source of truth for particle definitions.
- Composite effects under `packs/res/sfx` are part of particle integration,
  not a separate optional feature.
- Existing resource formats and paths must remain unchanged.
- Final and Release must use the same Core loaders, runtime definitions,
  preview builders, and renderer implementation.
- A malformed optional effect must not prevent the rest of a map from loading.
- No automated tests and no builds will be run by Codex at the user's request.
  The user will compile and perform runtime testing.

## Current-state findings

The resource pack contains 878 particle XML files plus visual, primitive,
texture, material, and animation dependencies. It also contains approximately
635 SFX XML files. Particle and SFX references occur in direct chunk particle
sections, UDO properties, model-related resources, weather/effect resources,
and more than a thousand chunk files.

The current implementation handles direct `particles` chunk sections and
loads only the particle definitions referenced by those sections. It does not
parse persistent `SFX_UDO`, `ParticleUDO`, or `ZoneEffectsUDO` placements and
does not resolve the actor/joint/event graph inside an SFX file. Consequently,
particles belonging to objects, items, weather, weapons, anomalies, and other
composite effects are absent even when their particle XML can be parsed.

The particle parser also reports the following resource types as unsupported:

- actions: `MatrixSwarm`, `NodeClamp`, and `Splat`;
- renderers: `AmpParticleRenderer`, `BlurParticleRenderer`,
  `MeshParticleRenderer`, `PointSpriteParticleRenderer`, and
  `VisualParticleRenderer`.

These are engine-level formats used across many particle definitions. They are
not a list of particle content categories.

## Architecture

The implementation will add one generic effect-resolution pipeline:

1. The chunk loader collects direct particle placements and persistent effect
   placements from supported chunk entities and UDOs.
2. A resource-reference resolver normalizes particle and SFX references and
   resolves them against the indexed `res` and `sys` mounts.
3. The SFX loader parses actors, joints, and events and expands every
   `ParticleSystem` actor into one or more particle placements with attachment
   metadata.
4. The particle loader parses the referenced particle definitions and all
   supported renderer/action data.
5. The preview builder resolves model or node attachments, prepares textures
   and mesh assets, and produces renderable emitter descriptions.
6. The runtime simulates each emitter and applies event-controlled behavior.
7. The renderer selects the correct sprite, trail, amp, blur, mesh, or visual
   path for every emitter.

Only resources reachable from the active map and its objects are instantiated.
Definitions and dependencies are cached by normalized logical path so repeated
references share parsed data and immutable GPU assets.

## Resource path resolution

A single Core helper will own particle and SFX path normalization. It will:

- accept `/` and `\\` separators;
- accept references with or without `res/` or `sys/`;
- append `.xml` only when no extension is present;
- preserve valid references already rooted at `res/` or `sys/`;
- prefer an existing normalized path instead of blindly prefixing `res/`;
- reject absolute paths, parent traversal, empty paths, and unsupported
  extensions;
- return the canonical logical path stored by `ResourceFileSystem`.

Chunk loading, SFX loading, particle loading, auditing, and future gameplay
callers must use this helper rather than implementing local path rules.

## Chunk and UDO discovery

`ChunkLoader` will retain generic data for the particle-bearing placements it
currently ignores. The first supported placement sources are:

- direct `particles` sections;
- `ParticleUDO`;
- `SFX_UDO`, including `sfx_persistent` and `sfx_oneshot` references;
- `ZoneEffectsUDO` particle or SFX references;
- entity or UDO properties containing declared particle/SFX resources when
  their schema identifies the property as such.

Each placement records its chunk ID, GUID when present, world transform,
logical resource reference, placement kind, persistence mode, and optional
model or node attachment. Outdoor chunk transforms and indoor transforms are
composed through the existing `Transform3x4` rules.

Schema definitions under `scripts/user_data_object_defs` and
`scripts/entity_defs` are used to identify typed particle/SFX properties. The
loader must not treat arbitrary strings containing `particle` or `sfx` as
resource paths. This avoids false positives and supports future maps that use
the same declared schemas.

One-shot UDO effects are described and loadable but are not repeatedly fired
by static map loading. They are activated once when the placement enters the
scene. Persistent effects remain active for the lifetime of the scene.

## SFX definitions

Add a Core SFX definition and loader for the resource grammar used by
`packs/res/sfx`. The loader preserves actor names and event target names and
supports the actor types needed to reach and place particle systems:

- `ParticleSystem` with its particle XML reference;
- `Model` and `DummyModel` when required as attachment providers;
- `Entity`, `HardPoint`, `ModelRoot`, and `Node` joint targets;
- light actors needed by a composite visual effect.

The event parser supports particle-affecting operations found in the pack:

- `ForceParticle`;
- `ClearParticles`;
- `RampTimeTriggeredParticles`;
- `ResetTimeTriggeredParticles`;
- `CorrectMotionTriggeredParticles`;
- `ParticleSubSystem` target filtering;
- `SetBasis`, `SetColour`, and `SetOrbitorPoint`;
- `SwarmTargets`;
- event timing data, including delays and TTL values.

Non-particle SFX events such as sound, post-processing, animation, decals, and
shockwaves are parsed as recognized metadata where needed to keep the graph
valid, but this task does not add unrelated audio, decal, or post-processing
engines. They must not prevent the particle actors in the same SFX from
running.

SFX references may be nested. Resolution detects cycles by maintaining the
active logical-path stack; a cyclic edge is logged and skipped without losing
unrelated actors.

## Model and item attachments

An expanded particle placement can target the SFX origin, `ModelRoot`, a named
node, or a named hardpoint. Attachments use model node transforms already
available from visual/model assets. The emitter's local transform is composed
with the current attachment transform every frame for moving objects and once
at scene creation for static objects.

Missing optional nodes fall back to `ModelRoot` and emit a warning containing
the SFX path, actor name, requested node, and model path. A missing model root
falls back to the placement transform. This keeps the rest of a composite SFX
visible while making bad content diagnosable.

The public effect-instantiation API accepts an SFX or particle logical path,
world transform, and optional attachment provider. Static map loading and
future item/gameplay systems use the same API; no item-specific particle list
is embedded in source code.

## Particle definitions

The particle definition model is extended to represent every action and
renderer type present in the current pack. Existing tolerant parsing remains:
unknown optional fields are ignored, while malformed values required by a
known type produce a resource-scoped diagnostic.

### Actions

- `MatrixSwarm` stores no serialized targets. Targets come from an SFX
  `SwarmTargets` event or an attachment provider. Particles are distributed
  across the resolved target transforms. With no targets it performs no
  positional override.
- `NodeClamp` reads `fullyClamp_`. Fully clamped particles follow the current
  emitter origin; relative mode applies the emitter's frame-to-frame
  displacement.
- `Splat` removes a particle when its movement segment intersects terrain or
  world collision geometry. The runtime receives collision through a narrow
  query interface rather than depending on Client classes directly.

Existing actions continue to retain their current semantics. Flare, barrier,
and collide behavior is not silently discarded when an SFX is used.

### Renderers

- `PointSpriteParticleRenderer` uses the sprite visual path with its own
  renderer identity and compatible point-sprite sizing/material fields. D3D11
  may expand it to camera-facing quads without changing content semantics.
- `BlurParticleRenderer` renders motion-oriented quads from previous to
  current particle position using the renderer's width, texture, material,
  and fog fields.
- `AmpParticleRenderer` builds the textured electrical strip between ordered
  particle positions using its step, width, texture, circularity, and material
  parameters.
- `MeshParticleRenderer` loads the referenced particle visual/primitives once
  and draws instances using particle position, rotation, spin, size, and tint.
- `VisualParticleRenderer` loads a normal visual and draws an instance per
  live particle with the same transform and tint rules.

Mesh and visual particle definitions retain their visual logical path and
material data. Missing visual or texture dependencies disable only the
affected emitter and produce a precise diagnostic.

## Runtime and event control

Each runtime emitter receives a stable instance identity, current attachment
transform, optional collision query, and optional swarm-target transforms.
Runtime state distinguishes continuous sources from event-forced particles.
SFX events address actor names and optional subsystem names without scanning
unrelated emitters.

Event timing is advanced from the same bounded frame delta already used by the
particle runtime. Loading a scene does not execute an unbounded event loop.
Persistent actors restart only when their resource semantics request it;
one-shot actors complete and remain inactive.

Camera-relative effects, including weather-style emitters, receive the current
camera transform through an attachment provider. Static world effects remain
in world space.

## Scene data and rendering

`SceneParticleEmitter` is extended with:

- normalized particle and optional SFX paths;
- effect/actor/subsystem identity;
- persistence and trigger mode;
- attachment description;
- renderer-specific texture or mesh data;
- immutable shared resource handles rather than duplicated mesh payloads.

Sprite-family particles continue through the particle GPU buffer. Mesh and
visual particles use cached immutable vertex/index/material resources and
per-particle transforms. Draw commands preserve renderer material mode,
depth-read behavior, fog selection, and additive/alpha/cutout blending.

The Final client and Release editor both call `LoadWorldPreview` and the same
renderer sources. Configuration-specific code must not contain separate
particle behavior.

## Caching and performance

The full pack must not be reparsed every time a map loads. Replace the current
mandatory world-load particle-pack audit with reachability-based loading.
Keep `ParticlePackAuditor` as an explicit diagnostic facility, updated to use
the shared resolver and recognize all supported types, but do not invoke it on
every normal scene load.

Cache layers are keyed by canonical logical path:

- parsed particle definitions;
- parsed SFX definitions;
- decoded textures and texture animations;
- loaded visual/primitives geometry;
- immutable GPU mesh resources.

Instance state, event timers, live particles, transforms, and attachment state
are never shared between placements.

## Error handling

Map loading distinguishes structural failures from effect-local failures:

- failure to initialize the resource filesystem or parse the space/chunk
  structure remains fatal;
- an invalid particle/SFX reference, unsupported future type, missing texture,
  missing visual, missing node, or cyclic SFX reference is local to the
  affected actor/emitter;
- local failures are logged once per canonical resource and do not abort
  unrelated map geometry or effects;
- log messages include map/space, chunk, placement GUID, SFX actor, particle
  subsystem, and canonical resource paths when those values are available;
- scene statistics report discovered, instantiated, disabled, and failed
  particle/SFX placements separately.

Unsupported future action and renderer types remain visible in diagnostics.
They are skipped safely rather than causing undefined rendering.

## Compatibility and future maps

No existing map or resource file is rewritten. Existing direct chunk particle
placements continue to work through the new common instantiation pipeline.
Future maps are supported when they use the same chunk, UDO/entity schema,
SFX, particle, model, visual, texture, and animation formats.

The design does not promise automatic semantics for a completely new engine
type that is absent from the current pack. Such a type is reported by name and
resource path so support can be added without changing map-specific code.

## Implementation boundaries

The change is expected to touch:

- Core resource-path utilities;
- chunk/UDO/entity placement structures and parsing;
- new SFX definitions and loader;
- particle action/renderer definitions and loader;
- particle runtime and its collision/attachment inputs;
- world scene effect placements and statistics;
- particle runtime/render data builders;
- scene render data, particle renderer, and particle shader paths;
- project files for newly added sources;
- both runtime shader copies under `game/Shaders` and
  `game/Studio/Shaders` when shader behavior changes.

Unrelated gameplay, server logic, audio playback, decals, and post-processing
are outside this task except where their SFX nodes must be tolerated so
particle actors can load.

## Delivery stages

Implementation is divided into dependency-ordered stages. Every stage leaves
the shared Final/Release code in a coherent state and prepares a distinct
runtime checkpoint for the user's later build and testing.

### Stage 1: Particle resource foundation

- centralize particle/SFX reference normalization;
- make particle loading and caching reachability-based;
- parse every action and renderer type present in `packs/res/particles`;
- add renderer/runtime data needed by point-sprite, blur, amp, mesh, visual,
  MatrixSwarm, NodeClamp, and Splat;
- remove the full-pack audit from the normal map-load path while retaining it
  as an explicit diagnostic facility.

Checkpoint: existing direct chunk particle placements can resolve every
particle resource format in the current pack.

### Stage 2: Direct placements and `ParticleUDO`

- extend chunk data to preserve typed UDO placements and their transforms;
- parse `ParticleUDO` from its declared schema;
- instantiate direct chunk particles and `ParticleUDO` through the same common
  effect-instantiation API;
- report missing resource and placement context without aborting the map.

Checkpoint: maps show both native `particles` sections and persistent or
one-shot particle UDO placements.

### Stage 3: SFX graphs and `SFX_UDO`

- add SFX actor, joint, and event definitions and the Core SFX loader;
- resolve `ParticleSystem` actors and their particle resources;
- implement event targeting, delays, TTL, force/reset/ramp/clear operations,
  and `SwarmTargets`;
- parse `SFX_UDO` properties including `sfx_persistent` and `sfx_oneshot`;
- instantiate SFX actors using the UDO world transform.

Checkpoint: static map objects represented by `SFX_UDO` play all particle
actors from their composite SFX resources.

### Stage 4: `ZoneEffectsUDO`, weather, and camera-relative effects

- parse `ZoneEffectsUDO` particle/SFX properties;
- resolve zone-effect identifiers through their declared resource data;
- connect weather SFX and camera-relative particle attachments;
- keep zone and weather lifetime separate from ordinary static UDO lifetime.

Checkpoint: zone, environmental, and weather particle systems are discovered
and placed without map-specific source lists.

### Stage 5: Object/item model attachments

- resolve `ModelRoot`, `Node`, and `HardPoint` joint targets;
- update attached emitter transforms from model nodes;
- expose the same effect-instantiation API to item, weapon, creature, and
  future gameplay systems;
- apply mesh/visual particle geometry and tint through shared cached assets.

Checkpoint: persistent object effects and dynamically requested item effects
can attach to the correct model origin or named node.

### Stage 6: Integration hardening

- verify project-file inclusion for every new source in the configurations
  that consume it;
- keep Final and Release shader copies synchronized;
- complete resource-scoped diagnostics and scene statistics;
- review all known resource types and placement sources for accidental
  `Unsupported` handling or silent drops;
- perform the static handoff checks listed below without building or running
  tests.

Checkpoint: the source is ready for the user's Release and Final build and
runtime validation.

## Verification handoff

Codex will not run tests or build either configuration. Before handoff Codex
will perform only non-executing checks: review the final diff, inspect project
file inclusion, compare the two shader copies, and scan for remaining known
unsupported particle/SFX type names in the loading and rendering paths.

The user will build and test:

- Release x64 editor map loading;
- Final x64 client world traversal;
- maps with direct chunk particles;
- maps with persistent and one-shot SFX UDOs;
- object/item effects attached to model roots and named nodes;
- weather and camera-relative particles;
- sprite, trail, amp, blur, point-sprite, mesh, and visual renderers;
- MatrixSwarm, NodeClamp, and Splat behavior;
- graceful handling of missing or malformed optional effect resources.
