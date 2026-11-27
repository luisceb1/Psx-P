# PSP presentation extension — 2026-11-27

Extend PSP GU adapter to a maximum 640x512 guest display window, with a 512x512 staging allocation. Modest modes keep prior source sampling; wide/tall windows stage at screen-fit dimensions. RGB24 uses packed RGB888 and quantizes to opaque RGB5551 for the existing framebuffer. Interlace displays a snapshot of stored VRAM rows, not exact field composition/timing. Row wrapping, aspect policy and visual correctness remain hardware validation gates.

Host C tests verify literal RGB5551/RGB24 conversions, VRAM wrap, downsample endpoints, and capacity rejection. Full integration adds three GU submission checks for 640x240, 640x480 interlaced snapshot, and 256x240 RGB24 with literal staged colors. These use original synthetic VRAM data, not MDEC or a commercial game. Presentation counters and staging values are not framebuffer readback or proof of exact display. Earlier checks preserved in the candidate; V5 remains recoverable by commit. No change to guest CPU or timing constants.

Reference: https://psx-spx.consoledev.net/graphicsprocessingunitgpu/ and pinned GPULib gpu.h/gpu.cpp.
