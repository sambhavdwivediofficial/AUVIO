# AUVIO Architecture

## Layers

- **utils** — `Logger`, `Config`. No dependencies on anything else; everything else depends on these.
- **core** — `Demuxer`, `Decoder`, `AudioEngine`, `VideoRenderer`, `PlaybackController`. Pure playback engine, built on FFmpeg and SDL2. Knows nothing about Qt.
- **platform** — `HardwareDetector`. One implementation per OS (Windows: DXGI + D3D11VA/DXVA2 probe; Linux: `lspci` + VAAPI/VDPAU probe), same interface on both.
- **ui** — `MainWindow`, `PlayerControls`, `PlaylistView`, `SettingsDialog`. Qt widgets. Depends on `core` and `utils`, never the other way around.

## Threading model

`PlaybackController` runs three threads per open file:

1. **Demux thread** — reads packets, routes them into bounded audio/video packet queues.
2. **Audio thread** — decodes audio packets, resamples, and pushes PCM into `AudioEngine`'s ring buffer. `AudioEngine`'s played-sample count is the master clock.
3. **Video thread** — decodes video packets, paces itself against the master clock (sleeps if early, drops frames if too far behind), then hands RGBA frames to `VideoRenderer`.

The queues are bounded, so a full queue blocks its producer — this naturally paces the demuxer to real playback speed instead of racing ahead.

## Video frame delivery to the UI

`VideoRenderer`'s callback fires on the video thread. `MainWindow` turns each frame into a `QImage`, deep-copies it, and emits a Qt signal — Qt automatically queues the delivery to the GUI thread, so no manual locking is needed on the UI side.

## Build & packaging

- CMake + vcpkg (`ffmpeg`, `sdl2`, `catch2` for tests)
- Windows: NSIS installer (`packaging/windows/installer.nsi`), icon/version resource (`packaging/windows/auvio.rc`)
- Linux: AppImage via `appimage-builder` (`packaging/linux/AppImageBuilder.yml`)
