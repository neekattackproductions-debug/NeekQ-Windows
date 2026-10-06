# NeekQ

A JUCE-based analog-modeled channel strip plugin (EQ + compression + saturation), built by **Neek Audio**. Originally scaffolded as "EQ5" — the product has been renamed to **NeekQ**, but some internal identifiers deliberately still say `EQ5` (see "Do not change" below).

- JUCE version: **8.0.14**, installed locally at `/Users/neek/Documents/JUCE 2/`
- Formats: **VST3, AU (macOS only), Standalone**. No AAX (Pro Tools support goes through a third-party VST3 wrapper, e.g. Blue Cat's PatchWork, not native AAX).
- Two parallel build systems exist on purpose — see "Build systems" below.

## Plugin structure

All plugin source lives in `Source/`:

| File | Purpose |
|---|---|
| `PluginProcessor.h/.cpp` | Audio processing core: APVTS parameter layout, `processBlock`, signal chain wiring (EQ → Saturation → Round/Punch/Juice compression → Limiter → Mix blend), auto-gain compensation, dry-signal delay for lookahead compensation. |
| `PluginEditor.h/.cpp` | UI: knob/button layout, attachments, custom LookAndFeel wiring, saturation-mode button group logic, timer-driven UI sync. |
| `FourKnobEQ.h` | 4-band EQ DSP. |
| `ThreeBandEQ.h` | 3-band EQ DSP (uses `M_PI` — requires `_USE_MATH_DEFINES` on MSVC, see CMakeLists.txt). |
| `OptoCompressor.h` | "Round" stage — opto-style compressor. |
| `FETCompressor.h` | "Punch" stage — FET-style compressor. |
| `PultecEQ.h` | "Juice" stage — Pultec-style passive EQ boost. |
| `Limiter.h` | Output limiter with lookahead; lookahead length is used to size the dry-signal delay for phase-accurate Mix blending. |
| `Saturation.h` | 5 saturation modes (Tape/Tube/Console/Fuzz/Germanium) driven by the hidden "drive" knob (`saturationDrive` parameter) on the logo's top-right screw. Increasing drive pushes further into the shaping curve rather than just increasing gain. |
| `HardwareLookAndFeel.h` | Custom `LookAndFeel_V4` — knob rendering (`metalBody`/`redBody`/default styles), custom `drawLabel`/`createSliderTextBox` (works around JUCE label-font bugs, see "Coding rules"), Akira Expanded button font, `drawButtonBackground` for the saturation-mode buttons. |
| `FaceMixLookAndFeel.h` | LookAndFeel for the Mix control. |
| `AkiraExpandedFont.h/.h` (data) | Embedded custom display font (binary-embedded, not looked up by name — see "Coding rules"). |
| `LogoData.h`, `FaceStencilData.h`, `logo.png`, `face_stencil.png` | Embedded UI art assets. |
| `Flanger.h` | **Orphaned/unused.** Was the original "easter egg" knob effect before it was repurposed into saturation drive. Not referenced anywhere and not in `CMakeLists.txt`'s `target_sources`. Left in place rather than deleted; do not wire it back in without being asked. |

## Build systems

### 1. Xcode (macOS local builds — day-to-day dev/testing)

Project: `Builds/MacOSX/EQ5.xcodeproj`. This is the **primary** way to build and install locally on this Mac.

**Important environment quirk:** Xcode.app on this machine lives at `/Users/neek/Downloads/Xcode.app`, not `/Applications/Xcode.app`, and `xcode-select` points at the bare Command Line Tools by default. Every `xcodebuild` invocation must override `DEVELOPER_DIR` explicitly rather than relying on system `xcode-select`:

```bash
export DEVELOPER_DIR=/Users/neek/Downloads/Xcode.app/Contents/Developer
```

Do not run `sudo xcode-select -s ...` to "fix" this — it changes system state and hasn't been asked for. Use the env var override per-command instead.

### 2. CMake (GitHub Actions CI — Windows + macOS cross-platform builds)

`CMakeLists.txt` at the repo root, consumed by `.github/workflows/build.yml` (matrix: `windows-latest`, `macos-latest`). This is **only** used for CI/Windows builds — it is not the local macOS dev loop. It uses `FetchContent` to pull JUCE 8.0.14 fresh on every run, so it's slower but requires no local JUCE install.

Push to `main` on `git@github.com:neekattackproductions-debug/NeekQ-Windows.git` (SSH remote — auth is via a dedicated deploy key at `~/.ssh/id_ed25519_neekq`, never a password/PAT) to trigger a build. Check run status with:

```bash
curl -s "https://api.github.com/repos/neekattackproductions-debug/NeekQ-Windows/actions/runs?branch=main&per_page=3"
```

Job *logs* are not fetchable via the API without admin auth (403) — ask the user to paste the failed step's output from the Actions UI instead of trying to download logs programmatically.

## Build commands

**Full universal Release build + auto-install (VST3 + AU + Standalone) in one step:**

```bash
cd /Users/neek/Desktop/EQ5/EQ5/Builds/MacOSX
DEVELOPER_DIR=/Users/neek/Downloads/Xcode.app/Contents/Developer \
  xcodebuild -project EQ5.xcodeproj -scheme "EQ5 - All" \
  -configuration Release ARCHS="x86_64 arm64" ONLY_ACTIVE_ARCH=NO build
```

The project's own "Install Target" / "Sign Target" run-script build phases copy the built VST3/AU straight into the system plugin folders (see "Install locations") as part of this build — no manual `cp` needed. The Standalone `.app` is left in `Builds/MacOSX/build/Release/NeekQ.app` and is **not** auto-copied to `/Applications`; copy it there manually only if asked.

**Debug build** (faster, arm64-only, for quick local iteration — do not distribute this build):

```bash
cd /Users/neek/Desktop/EQ5/EQ5/Builds/MacOSX
DEVELOPER_DIR=/Users/neek/Downloads/Xcode.app/Contents/Developer \
  xcodebuild -project EQ5.xcodeproj -scheme "EQ5 - All" build
```

**Verify a build is a true universal binary before distributing:**

```bash
lipo -info ~/Library/Audio/Plug-Ins/VST3/NeekQ.vst3/Contents/MacOS/NeekQ
# must print: x86_64 arm64
```

An arm64-only build loads fine in Logic/native hosts but is rejected by 64-bit wrapper checks in tools like Blue Cat's PatchWork (this exact bug was hit and fixed once already — always build Release + universal ARCHS before packaging for beta/Pro Tools use).

## Install locations

| Format | Path |
|---|---|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/NeekQ.vst3` |
| AU | `~/Library/Audio/Plug-Ins/Components/NeekQ.component` |
| Standalone | `Builds/MacOSX/build/Release/NeekQ.app` (build output only — copy to `/Applications` manually if needed) |

Windows (from CI artifacts): `NeekQ.vst3` → `C:\Program Files\Common Files\VST3\`.

## Coding rules / conventions

- **Property-flag styling pattern**: per-instance LookAndFeel behavior (not per-class) is set via `component.getProperties().set("propName", true)` and read back in `HardwareLookAndFeel`/`FaceMixLookAndFeel`. Existing flags: `hideArc`, `bigKnob`, `greenWhenPositive`, `metalBody`, `redBody`, `useCaptionFont`, `useHackFont`. Follow this pattern for new per-component style variants rather than adding new LookAndFeel subclasses.
- **`Label::setFont()` is silently ignored by default JUCE label drawing** — `LookAndFeel_V2::drawLabel()` always calls `getLabelFont(label)` regardless of an explicitly-set font. `HardwareLookAndFeel::drawLabel()` overrides this and checks the `"useCaptionFont"` property to decide which font to honor. If you add a new label that needs an explicit font, set this property rather than relying on `setFont()` alone.
- **`Slider::setTextBoxStyle()` must be called *after* `addAndMakeVisible()`**, not before. It internally triggers `createSliderTextBox()`, which resolves the active LookAndFeel via the component's current parent chain — called too early, it silently resolves to JUCE's *default* LookAndFeel instead of `HardwareLookAndFeel`, and any custom text-box styling/property-copying is lost with no error. If a slider must be styled before being added to its parent, call `slider.setLookAndFeel(&hardwareLookAndFeel)` explicitly first.
- **Custom fonts are embedded as binary data**, not looked up by system font name (`juce::Typeface::createSystemTypefaceFor(data, size)`), because "Akira Expanded" only ships a "Super Bold" style with no "Regular", which fails silent system name-lookup. Follow this pattern for any future embedded font.
- **Decorative "imperfection" randomness must be stable, not per-frame** — seed `juce::Random` from a fixed value like a knob's screen position, never from a global/default-seeded RNG, or knurled-knob-style decorations will flicker while dragging.
- **Saturation drive is deliberately not gain-compensated at the shaping stage** — `Saturation::processSample` multiplies drive into the pre-shaping gain but keeps the normalization divisor fixed to the mode's base drive, so higher drive pushes further into clipping rather than just getting louder. This is intentional; don't "fix" it by rescaling the divisor unless asked.
- **Saturation-mode buttons are a mutually-exclusive group bound to a single `AudioParameterChoice`**, not five independent `AudioParameterBool`s, and not JUCE `ButtonAttachment`s (JUCE doesn't support attaching a button group to a single choice parameter directly). Each button's `onClick` reads/writes `saturationParam` directly, and `PluginEditor::updateSaturationButtonStates()` (driven by a 15Hz `Timer`) keeps button toggle state in sync for preset recall/automation. Follow this pattern for any future mutually-exclusive button-group parameter.
- **Bundle identifiers stay `com.yourcompany.EQ5`** — never renamed, by longstanding deliberate choice, independent of the product-name/manufacturer-string fixes.

## Do not change

- **`EQ5.jucer`** — badly stale (lists only 4 of ~20 source files, only has a macOS exporter, still says `name="EQ5"`). Do **not** open it in Projucer and re-save/regenerate — that would blow away manual fixes made directly in `Builds/MacOSX/EQ5.xcodeproj` and `JuceLibraryCode/` (product rename, manufacturer strings, Info.plist fixes, universal-binary settings). The Xcode project is the actual source of truth for the macOS build, not the `.jucer` file.
- **`Info-AU.plist`, `Info-VST3.plist`, `Info-Standalone_Plugin.plist`** (in `Builds/MacOSX/`) — these are static Projucer-generated templates that do **not** get regenerated automatically; if the product name, manufacturer, or bundle display strings ever need to change again, they must be hand-edited here directly (this has already bitten us once — Logic Pro showed stale `"EQ5"` / `"yourcompany"` because these files weren't touched by the original rename).
- **`JuceLibraryCode/`** — Projucer/JUCE-generated glue code (mostly `include_juce_*.cpp/.mm` module umbrella files plus `JucePluginDefines.h`, `JuceHeader.h`). Don't hand-edit the `include_juce_*` files. `JucePluginDefines.h` *has* been intentionally hand-edited for manufacturer/product strings since `.jucer` is stale — if the `.jucer` is ever regenerated, re-verify these.
- **Bundle identifiers** (`com.yourcompany.EQ5` in the Xcode project and Info.plists) — leave as-is; this is deliberate, not an oversight.
- **`Flanger.h`** — orphaned, unused, kept for reference. Don't delete without being asked; don't wire it back into the signal chain (the easter-egg knob is saturation drive now, not a flanger).
- **CMake's `BUNDLE_ID`** in `CMakeLists.txt` — kept as `com.yourcompany.EQ5` to match the Xcode build for consistency; same rule as above.

## Build & test procedure

1. Make source changes in `Source/`.
2. Rebuild via the Xcode `"EQ5 - All"` scheme, **Release**, universal `ARCHS="x86_64 arm64"`, `ONLY_ACTIVE_ARCH=NO` (see "Build commands"). This auto-installs VST3 + AU to the system plugin folders.
3. Verify the install actually updated (don't assume — the install script runs even when nothing changed, so confirm the binary is genuinely fresh):
   ```bash
   md5 Builds/MacOSX/build/Release/NeekQ.vst3/Contents/MacOS/NeekQ ~/Library/Audio/Plug-Ins/VST3/NeekQ.vst3/Contents/MacOS/NeekQ
   ```
   Checksums should match, and both should be newer than your source edit.
4. Confirm it's a true universal binary before calling it done: `lipo -info` on the installed binary should print `x86_64 arm64`.
5. Load/reload in a real host to sanity-check: Logic Pro (AU) or a VST3 host. Existing running instances of the plugin in a DAW may need the track/instance removed and re-added, or the DAW restarted, to pick up a rebuilt binary — a stale in-memory instance won't hot-reload.
6. For **Windows** changes: push to `main` on the `NeekQ-Windows` repo (SSH remote), then poll the GitHub Actions run status via the API. Since macOS (CMake) and Windows both build from the same `CMakeLists.txt`, a macOS CI failure usually means a genuine CMake/shared-code problem, while a Windows-only failure is usually MSVC-specific (e.g. missing `_USE_MATH_DEFINES` for `M_PI`, a header MSVC is stricter about, etc.) — check what's *different* between the two job logs first.
7. Before packaging a beta build for distribution, always rebuild Release/universal fresh rather than reusing a Debug or arm64-only artifact — Debug/arm64-only builds have caused real compatibility failures in third-party hosts (see PatchWork "not 64-bit" issue).
