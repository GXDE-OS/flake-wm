# wlroots compatibility sources

`0.17/` mirrors wlroots source-tree paths and contains complete source files,
not patches. These files carry the compositor-specific behaviour inherited from
`open-kylin-wlroots@950dbeb6`.

At configure time, `cmake/PrepareWlroots.cmake` copies the pristine official
`libs/wlroots` tree to the build directory and then overlays `0.17/`. Only the
generated build tree is compiled.

Keep all backports and removal conditions synchronized with
[`WLR_UPGRADE.md`](../../../WLR_UPGRADE.md).
