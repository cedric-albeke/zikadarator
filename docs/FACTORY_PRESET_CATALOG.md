# ZIKADARATOR V1 Factory Preset Catalog

The V1 factory bank contains exactly 50 embedded full-state presets. `INIT` is the only empty state; the other 49 provide distinct six-lane sequencer patterns using the implemented lane preset range 5-20.

The original eight presets retain their existing state builders and sound. The additional 42 presets use a declarative pattern catalog with explicit step masks, primary/alternate lane effects, global mix values, and bounded user-slot tuning.

## Utility (1)

- INIT

## Glitch (7)

- NEON GATE
- CIRCUIT TEETH
- SIGNAL SKIP
- PIXEL RIOT
- BROKEN CLOCK
- DIGITAL SHIVER
- STUTTER CODE

## Groove (6)

- OFFBEAT ENGINE
- POCKET CUTTER
- GHOST NOTES
- FOUR FLOOR
- SYNC BUG
- BACKBEAT LASER

## Loop (7)

- LOOP CHOP
- RATCHET LOOP
- BACKSPIN GRID
- HALF TIME HAZE
- REVERSE POCKET
- LOOP LADDER
- TAIL SPIRAL

## Filter (8)

- FILTER CUTS
- NOTCH MOTION
- ACID CICADA
- LOWPASS DRIFT
- RESONANT STEPS
- COMB RUNNER
- NOTCH PARADE
- BAND SCANNER

## Delay (6)

- DELAY PULSE
- PHOSPHOR RAIN
- NIGHT TRANSMISSION
- DUB BLOOM
- SPACE DEBRIS
- GLIDE MATRIX

## Ambient (4)

- SPACE BLOOM
- ORBITAL DUST
- EMPTY STATION
- AFTERGLOW

## Texture (6)

- CRUSH GRID
- BIT DUST
- RING STATIC
- CRUSH BLOSSOM
- PITCH MOSAIC
- DIGITAL RUST

## Motion (5)

- WOBBLE BUS
- TREMOR FIELD
- PULSE ENGINE
- SLOW SWARM
- PANIC SIGNAL

## V1 Contract

- Factory names and serialized states are unique.
- Factory navigation order is stable and does not change when favorites or recents change.
- Every state contains the complete 121-parameter APVTS schema and one complete 6x16 step / 6x4 user-slot sequencer state.
- Active steps use only implemented lane preset indices 5-20.
- APVTS active gates and sequencer active flags match.
- Every factory state survives apply/export and binary host-state roundtrips.
- Factory states reset lane mute/solo and use finite, bounded slot values.
- User presets cannot take a factory name. Saving the same sanitized user filename performs an explicit atomic update and reports that result in the browser.
- Legacy user files whose names now collide with the factory bank remain accessible under a unique `(USER)` display name.
- Incomplete, duplicate, out-of-range, or structurally invalid user states are rejected before save and ignored during discovery.
- Browser selection is preserved by stable preset ID across refresh and reorder operations.

Scratch and vinyl are not represented by substitute names in this bank. They require dedicated transport-aware DSP before they can become honest V1 effects. `WOBBLE BUS` combines the existing wobble envelope and modulated-delay paths; it is not presented as wow/flutter emulation.
