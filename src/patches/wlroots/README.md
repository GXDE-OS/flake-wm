# Historical wlroots 0.17 compatibility sources

`0.17/` mirrors wlroots source-tree paths and contains complete source files,
not patches. These files carry the compositor-specific behaviour inherited from
`open-kylin-wlroots@950dbeb6`.

The old 0.17 build used `cmake/PrepareWlroots.cmake` to overlay these files on a
build-directory copy of the official sources. That mechanism is **not used by
the current 0.20.2 build**: `libs/wlroots` is compiled without modifications.

Keep all backports and removal conditions synchronized with
[`WLR_UPGRADE.md`](../../../WLR_UPGRADE.md).
Keep this historical overlay until every behaviour has been classified and
tested; do not copy these files
over wlroots 0.20.2. Independent adapters now live directly in `src/patches`.
See `WLR_UPGRADE.md` for current migration status and remaining work.
