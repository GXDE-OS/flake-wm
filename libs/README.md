# Third Party Library Vendor Information
## Wlroots
* **Upstream**: https://gitlab.freedesktop.org/wlroots/wlroots
* **Tag**: `0.17.4`
* **Version**: `0.17.4`
* **Commit ID**: `a2d2c38a3127745629293066beeed0a649dff8de`
* **Commit date**: Thu Jun 27 20:25:08 2024 +0200
* **Import date**: 2026-10-03
* **License**: [MIT License](./wlroots/LICENSE)
* **Note**: This directory is a pristine copy of the official tag. GXDE/openKylin compatibility sources live in [`src/patches/wlroots/0.17`](../src/patches/wlroots/0.17) and are documented in [`WLR_UPGRADE.md`](../WLR_UPGRADE.md). CMake copies this tree into the build directory, overlays those compositor-owned source files there, and links the resulting static library without installing it system-wide.
