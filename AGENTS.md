# Development rules

- Preserve upstream authorship, history, licenses and notices. No BIOS or commercial disc images in Git, CI artifacts or issues.
- Keep changes small, reproducible and reviewable. Report actual build and hardware status separately.
- Start by making the historical interpreter build; do not enable the incomplete dynarec or change timing constants blindly.
- Separate platform storage, UI, disc parsing, CPU, GPU and SPU changes. Add synthetic regression tests where practical.
- Do not claim compatibility, audio, speed or hardware validation without measurements. Never overwrite user memory cards or existing PSX-P installation.
- Record the SDK/container version and commit in each test report. Before distribution audit bundled assets and libraries and provide corresponding source.
