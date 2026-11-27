# Native PSP audio diagnostic — 2026-11-27

V11 separates audio from the full V10 diagnostic. The original null driver remains the default for full-test. New audio-test EBOOT links the same pinned portable PCSX ReARMed SPU and a native PSP PCM backend, no guest CPU, game, BIOS, SDL or hardware timing changes.

Backend: signed interleaved stereo PCM at nominal 44.1 kHz; 512-frame aligned output buffer, 4096-frame bounded ring, nonblocking sceAudioOutputPanned with quiet hardware volume, rest-length check before buffer reuse, explicit cache writeback. Pump on feed and diagnostic polling; no worker or sample-rate adaptation. Overflow drops newest frames and is counted; partial tail is discarded at close and counted. No audio pacing or emulated timing change. Release has bounded polling; release failure stays visible.

Test: quiet generated triangle tone left then right, then original SPU mixing of a short synthetic looping ADPCM block configured by actual SPU register writes, separate synthetic mixer cycle clock. Metrics include accepted/nonzero/dropped frame counts, submitted blocks, errors, busy and release status. They do not establish audible sound, rate accuracy or real game compatibility. Host tests verify ring order, partial, wrap and overflow. On PSP listen for the two phases and retrieve PSXPRV_AUDIO.TXT. No game audio, no distribution or public binaries. No firmware Pause test; retain null backend until physical validation and lifetime/pacing review.

PSPSDK API reference: https://github.com/pspdev/pspsdk/blob/master/src/audio/pspaudio.h
