# Release QC checklist

- CI: MPC ARM32/ARM64 + macOS Standalone ARM64; inspect factory, articulation, stereo, switching, 60-second streaming CSV, performance and acoustic preview results.
- Listen to all 100 exported Factory WAVs; evaluate registers, dynamics, note-off, loop seam, stereo balance, clipping and noise. Record reviewer, device, date, preset, status and notes.
- On actual MPC ARM32 and ARM64 and Apple Silicon, test 44.1/48 kHz, available buffers, rapid presets, sustain, repeated notes, polyphony and 60-second streaming from supported SSD/SD. Capture CPU, total process RAM and missing reads; zero audio gaps required before enabling experimental streaming by default.
- On 13-inch Mac and MPC screen, inspect panel and Studio text, accents, clipping, overlaps, keyboard, export, focus, and real control actions. Static checks are not screenshot validation.
- Review acoustic provenance and audition all imported notes. No new proprietary recording rights assumed. CC0 evaluation is a separate optional artifact, not a replacement of Factory.
- Commercial release gate: completed human listening and hardware QA, verified installer/signing/notarization if distributing signed macOS app, install/uninstall and rollback instructions. MPC firmware compatibility to be verified separately.
