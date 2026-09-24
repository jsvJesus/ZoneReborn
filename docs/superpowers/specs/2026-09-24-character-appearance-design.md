# Character Appearance Creator Design

Date: 2026-09-24

## Goal

Implement the second stage of character creation: the `Choose appearance`
submenu, a head-focused camera, live facial customization, and persistence of
the resulting face state. The implementation must retain the visual language
of the existing frontend and use the original game resources and behavior as
the reference.

## Scope

This stage includes:

- face-form generation and rendering;
- skin colour, age, and details;
- eye colour;
- hair, moustache, and beard styles;
- hair, moustache, and beard lengths;
- hair colour and unshaven amount;
- eyebrow style, position, and rotation;
- tattoo style and colour;
- reset, randomize, apply, and back behavior inside the appearance submenu;
- a close-up head camera while the submenu is open;
- saving and loading the complete appearance state.

Direct mouse sculpting of individual facial areas is intentionally excluded
from this stage. The face form is generated with the original FaceSaver rules
and can be randomized from the Face category. This keeps the data and renderer
compatible with later direct sculpting without requiring the original
off-screen colour-picking system now.

Automated tests are excluded at the user's request. The implementation will
still be syntax-checked, compiled, and manually verified through targeted
runtime logging and inspection.

## Source of truth

- `charMakerCfg.json` remains the clothing-only creator configuration.
- `charMakerFaceCfg.json` is the UI/source configuration for facial choices.
- `CLOTH.pyson` remains the item/model source for hair, moustache, and beard
  item types.
- `FaceSettings.pyc_dis` defines face bones, paired handles, deformation axes,
  and limits.
- `FaceSaver.pyc_dis` defines random generation and the packed face-form
  representation.
- `AvatarDummy.pyc_dis` defines mapping from UI choice groups to personality
  fields and face slots.
- `StalkerModel.pyc_dis` defines material behavior and length mapping.
- `DummyCameraFlyer.pyc_dis` defines the reference head-camera framing.

The Python disassemblies are design references only. The active implementation
will be native C++ and the WebView frontend; no Python runtime is introduced.

## User flow

1. The main creator continues to show the full character.
2. Selecting `Choose appearance` snapshots the current `FaceState`, opens the
   appearance layout, and requests appearance camera mode.
3. The camera frames the head using the original target height, distance, and
   field-of-view as the starting values, adapted to the current stage
   transform.
4. The left panel lists `Face`, `Skin`, `Eyes`, and the hair subcategories.
   The right panel displays the selected category's original icons, swatches,
   and sliders.
5. Every control updates the native creator state immediately. Item-style
   changes rebuild character model parts; scalar and colour changes update
   face rendering state without losing clothing selections.
6. `Reset` restores the appearance snapshot taken when the submenu opened.
7. `Random` generates a complete valid face state and valid facial item
   selection.
8. `Apply` retains the edited state and returns to the main creator camera.
9. `Back` restores the snapshot and returns to the main creator camera.
10. The final main-screen `Create` operation persists clothing and face state
    together. Opening the appearance submenu never writes a character file.

## Frontend design

The appearance screen is another mode of the existing creator screen, not a
separate visual theme. It reuses the current top account/balance/language bar,
font stack, red navigation marker, translucent black panels, spacing, and
responsive sizing.

The left column contains category navigation and the submenu actions. The right
column is a translucent options panel. Picture choices use the existing assets
under `soGUI/frame/character_creation/face`. Colour values and weights come
from `charMakerFaceCfg.json`; donor-only entries are omitted for normal
creation, matching the original screen.

Sliders expose values from 0 through 100. The frontend sends semantic choice
group names such as `Age`, `HairLength`, and `EyebrowsRotation`; conversion to
the native 0 through 255 representation occurs in C++.

Frontend messages:

- `character_face_open`
- `character_face_value`, carrying a choice-group name and numeric value
- `character_face_random`
- `character_face_reset`
- `character_face_apply`
- `character_face_cancel`

The native side sends the normalized current face state after open, reset, and
random operations so the frontend never becomes the authoritative state.

## Native face state

`FaceState` remains the authoritative in-memory representation and is extended
with explicit style item IDs:

- `hairStyle`
- `moustacheStyle`
- `beardStyle`

Existing scalar, colour, eyebrow, and tattoo fields remain. `faceForm` changes
from `vector<int16_t>` to `vector<uint64_t>` because the original FaceSaver
format is an array of packed unsigned 64-bit words.

Face-form encoding and decoding are implemented as a dedicated native module.
It ports the eight-bits-per-value quantization and limits from FaceSaver and
FaceSettings. Internally, decoded form data is represented as per-handle
translation/scale transforms, with mirrored transforms applied to paired
bones.

Reset produces stable defaults. Randomization uses the original weighted
colour/style choices and bounded face-form distribution. Donor-only options
are excluded.

## Model composition

Hair, moustache, and beard continue to be ordinary character items in slots
130, 131, and 132. The appearance controller validates style IDs against the
allow-list parsed from `charMakerFaceCfg.json`, then equips or clears the
corresponding slot. This allows the existing model composer to load the real
models from `CLOTH.pyson`.

Choosing item ID zero clears the slot. Model rebuilds preserve the current
clothing and all non-model face values.

The catalog gains only the facial metadata required by rendering: substrate
texture, tint/material name, and length limits. Facial groups are kept separate
from clothing creator groups so `charMakerCfg.json` remains clothing-only.

## Facial deformation

The character animator receives the decoded face transforms. For each animated
frame it:

1. samples the idle animation's local node transform;
2. pre-multiplies the configured face morph for matching facial bones;
3. resolves the hierarchy;
4. builds the skin palette and performs the existing CPU skinning.

This mirrors `FaceController.Handle.apply_update`, where `morph_disp` is
pre-multiplied into the local bone transform. Paired bones share the same packed
handle value, matching the original symmetric editing behavior.

Missing facial bones do not fail an otherwise valid model; they are logged and
ignored. Malformed packed face data falls back to the default face form.

## Facial materials

The current model renderer only supports one diffuse texture per primitive
group. It will be extended with an optional facial-material payload. The model
builder identifies the original head, eyes, hair, moustache, and beard
materials by their visual material/effect metadata and attaches the current
`FaceState` values.

The facial shader path supports:

- skin-colour modulation;
- eye-colour modulation;
- hair-colour modulation;
- age/normal blend where the referenced normal texture is available;
- details/scar alpha;
- unshaven alpha;
- eyebrow texture, vertical shift, and angle;
- tattoo texture and colour;
- substrate texture and length shift for hair, moustache, and beard.

Original DDS resources from the model material, `CLOTH.pyson`, brow directory,
and battle-mark directory are used. Non-facial materials continue through the
existing world shader path unchanged.

Material-only changes update renderer material state directly. Style changes
that add or remove model files trigger a character rebuild.

## Camera

Application state gains creator camera modes: full body and face. Entering face
mode derives a camera from the current character world transform, targets the
head around 1.63 metres above the character origin, starts near the original
0.63 metre distance, and uses the original 0.7-radian field of view. Returning
to the main creator restores the stage camera exactly.

Character right-drag rotation remains available. Face mode does not implement
the original scroll zoom in this stage.

## Persistence and compatibility

Character files advance to `CHARACTER_V2`. The format stores the existing ID,
name, and clothing appearance followed by every `FaceState` scalar/style field
and the count plus values of packed `uint64_t` face-form words.

The loader accepts both versions:

- `CHARACTER_V1` loads its clothing and supplies a default face state;
- `CHARACTER_V2` validates all face values and facial item IDs before applying
  them.

New files are written atomically using the service's existing temporary-file
replacement flow. Invalid or truncated V2 face data rejects the profile rather
than partially applying it.

## Error handling

- Failure to load `charMakerFaceCfg.json` disables the appearance entry and
  reports a localized frontend error without affecting clothing creation.
- Unknown choice groups and invalid style IDs are rejected at the frontend
  boundary and logged.
- Missing optional facial textures degrade only the associated effect; base
  head rendering remains available.
- Missing required head/model assets fail character rebuild with the existing
  fatal renderer error path.
- Cancelling or leaving the creator restores the appropriate snapshot and
  never writes a profile.

## Verification

Without adding automated tests, verification consists of:

- JavaScript syntax checks for all changed frontend scripts;
- whitespace/diff validation;
- a full Final x64 build;
- runtime checks that every face control changes the rendered character;
- reset/apply/back snapshot behavior;
- face camera enter/exit behavior;
- create, restart, and V2 profile reload preserving the face;
- loading an existing V1 profile with a valid default face.
