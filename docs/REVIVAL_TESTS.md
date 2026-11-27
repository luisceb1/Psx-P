# Hardware test protocol

Install each experimental build in a separate PSP/GAME/PSXPRV/ directory. Back up memory cards and save states first. Use a user-supplied lawful BIOS and homebrew test discs or personal backups where permitted. Never upload game or BIOS files.

Record: commit, SDK version, PSP model, firmware, storage root, media format, test title/version, boot result, FPS and emulated speed if measurable, audio, graphics, save/load, exit result and log. Mark unavailable measurements as unknown.

Test internal ef0:/ and removable ms0:/ separately. Check missing BIOS, empty directory, unsupported file, long name, failed reads, and exit from menu and gameplay. Run PAL and NTSC tests separately. Keep CPU clock and other options identical for comparisons. Never present a successful menu boot as a successful game boot.
