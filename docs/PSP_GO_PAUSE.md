# PSP Go Pause Game investigation — 2026-11-27

Do not confuse HOME -> Exit (confirmed fixed on v0.0.2) with PSP Go HOME -> Pause Game and XMB Resume Game, which is a firmware snapshot feature. User reports the pause fault persists; exact firmware revision and whether failure is on creation or restoration are not established. No change to stable main or firmware is justified yet.

## External evidence
- [ARK-5 #65](https://github.com/PSP-Arkfive/ARK-5/issues/65): regression across multiple PSP retail titles; maintainer closed as working on latest build on 2026-11-02.
- [ARK-5 #83](https://github.com/PSP-Arkfive/ARK-5/issues/83): freeze after resuming reported on PSP Go; reporter observed improvement disabling Inferno Cache. Closed as duplicate, reporter disputed equivalence. This is a distinct possible symptom and not proof of the cause in PSX-P. Record exact ARK revision and settings before any reversible comparison. No firmware downgrade, flashing, or configuration changes performed.
- [PSPSDK power callbacks](https://github.com/pspdev/pspsdk/blob/master/src/power/psppower.h): ordinary suspend/resume event flags. No guarantee that firmware snapshot emits these callbacks.

## Independent probe
Build creates pause-test/EBOOT.PBP, with debug screen, controller, callback thread and a small RAM marker, no core/CD/BIOS/SDL/GU/SPU. No high-memory request. No writes while waiting idle; logs opened, appended and closed on explicit button actions/exit. Test in separate directory without valuable saves; do not leave another important paused game in the XMB as its slot may be replaced. First confirm normal HOME Exit, relaunch, press X to arm, choose Pause Game, observe whether XMB Resume Game appears; select it, press Circle to check the marker and log notifications. Log PSXPRV_PAUSE.TXT. A new session with missing marker is not a successful resume. Separately test ordinary power-switch suspend; do not equate with the snapshot. Do not infer success from firmware menu alone or absence of callbacks.

If minimal probe also freezes, investigate firmware/settings first; if probe passes and emulator fails, narrow to emulator resources (open disc handles, GU, memory use, callbacks, dynarec cache) using isolated stages. No claim of a fix, no public binary. Physical testing pending.

## Integration candidate V5
Add native digital PSP input to the PCSX4ALL PAD frontend (active-low), synthetic neutral/combined/all-button mapping and actual SIO MMIO poll tests for port 1/neutral port 2 and IRQ. Port 2 is neutral rather than claimed disconnected. PSP L/R map to PS1 L1/R1, no L2/R2, L3/R3, analog or rumble yet. Physical controller validation pending. Independent pause probe has no effect on full-core execution.
