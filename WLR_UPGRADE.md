# wlroots 兼容层与升级账本
## Q: 为什么仓库里有两个Wlroots？
你可能会注意到我们有两个Wlroots：`libs/`和`src/patches`下；第一个是原版的Wlroots 0.17.4, 第二个是一小分Kylin Wlroots的子集，用于覆盖第一份内同名文件

---

以下内容是给维护者和AGENTS看的，语气上是以AGENT为听众做的，便于直接把以下内容当PROMPT：

## 基线与约束

- Vendor 基线是官方 wlroots `0.17.4`，提交
  `a2d2c38a3127745629293066beeed0a649dff8de`。
- `libs/wlroots` 必须与该官方提交保持一致，不得直接回填功能或修复。
- 历史行为基线是 `open-kylin-wlroots` 的 GXDE 分支提交
  `950dbeb6cc3701832b3623fdd4bc51c9a9a37f6e`（`0.17.5-gxde2`）。
- 所有相对官方版本的代码都存放在 `src/patches/wlroots/0.17`。目录结构与 wlroots
  对齐，CMake 在构建目录创建官方源码副本后，用这些合成器内源码覆盖对应文件，
  不修改 Vendor 本体。
- openKylin/GXDE fork 自带的 Debian 打包文件和 README 没有进入兼容层；它们不影响
  wlroots 运行时，并且本仓库已有自己的 Debian 元数据和 Vendor 说明。

以后新增任何从较新 wlroots 回填的改动，必须在本文件登记：上游提交、首次包含该
改动的发布版、本地使用原因和删除条件。不得只在 overlay 源码里留下代码而不登记。

## 当前兼容源码

### `src/patches/wlroots/0.17/backend` 与 `include/backend`

DRM、libinput 和 session 兼容行为。

已成为官方实现的回填：

| 功能 | fork 提交 | 官方提交 | 首个官方版本 | 升级动作 |
|---|---|---|---|---|
| 查询 multi-GPU DRM parent | `33fa69dcb` | `fc7a0b93` | 0.18.0 | 0.18+ 改用上游实现 |
| atomic tearing page-flip | `bd78f98bb` | `00b869c1` | 0.18.0 | 0.18+ 改用上游实现 |
| multi-GPU buffer format 检查 | `c395ee403` | `bdc75d05` | 0.18.2 | 0.18.2+ 改用上游实现 |
| 扫描 connector 时不重新分配 CRTC | `be057d9bf` | `96ad414e` | 0.19.0 | 0.19+ 改用上游实现 |
| 忽略重复 DRM udev add 事件 | `134eec410` | `5eed5d62` | 0.19.0 | 0.19+ 改用上游实现 |

仍为 openKylin/GXDE 特有的改动：

- `6a9aad0fd`：legacy DRM 缺少 plane object 时补建 cursor plane。
- `158d7e1e9`：VT 切换期间记录 hotplug，并按实际 connector/CRTC 状态恢复。
- `0c8cd8e9e`：legacy modeset 时先 DPMS off，成功设定 CRTC 后再 DPMS on。
- `00856a42d`：session 恢复后立即 dispatch libinput 积压事件。
- `43bae57e6`：swipe/pinch 使用未加速 delta。
- `ff223deb5`：启动时没有输入设备也保留 libinput backend。
- `3ae3727c9`：GPU 枚举时避免优先选择 USB 显卡。
- `cc68f6d83`：处理 libseat event source 的 HUP/ERROR。
- `719be0410`：忽略 libinput 未知 switch 类型，避免使用未初始化枚举值。

这些特有行为在升级时不能因旧 overlay 文件不能复用而静默丢弃；必须分别确认新上游是否已有
等价逻辑，或在实际硬件上验证不再需要。

### `src/patches/wlroots/0.17/render` 与 surface/output/cursor 源码

renderer、allocator、surface/output/cursor 和 pointer-constraint 修复。

已成为官方实现的回填：

| 功能 | fork 提交 | 官方提交 | 首个官方版本 | 升级动作 |
|---|---|---|---|---|
| Vulkan `blend none` clip | `c842842df` | `6cc80472` | 0.18.1 | 0.18.1+ 删除 |
| 每次 surface commit 重置 `current.committed` | `d5bdd2117` | `df27b29d` | 0.18.0 | 0.18+ 删除 |
| pointer constraint 的 `cursor_hint.enabled` 与 committed 重置 | `df72a69ea`, `9d76ad7e4` | `85f44f36`, `da5f53b4` | 0.18.0 | 0.18+ 删除 |
| 相同 keymap 不重复发送 | `31eb9216e` | `1e7baefe` | 0.19.0 | 0.19+ 删除 |
| output disabled 时仍更新 cursor 状态 | `4a529eeef` | `c1938f79` | 0.19.1 | 0.19.1+ 删除 |
| 避免双硬件光标 | `7eba53471` | `aa904ccf` | 0.19.1 | 0.19.1+ 删除 |
| GBM BO export 成功后再挂入 allocator list | `0db30edd0` | `b0c886ec` | 0.20.0 | 0.20+ 删除 |

仍为 fork 特有的改动：

- `db757c338`：运行时探测 `gbm_bo_get_fd_for_plane`，兼容旧 GBM runtime。
- `9b1de2a1a`：关闭已导入 DMA-BUF handle 失败时不拒绝 buffer（VMware）。
- `7638c23a1`：screencopy 用 output commit 的 ENABLED 状态代替 enable listener。
- `e6ab250cc`：修正 Pixman read-pixels 的目标偏移。
- `f61416b0c`：cursor surface 没有 client texture 时从当前 buffer 建 texture。
- `2b318d480`：只为 SHM surface 保留 `wlr_client_buffer` 上传路径。
- `bc9f0873b`：effective resolution 对缩放结果向上取整。

### `src/patches/wlroots/0.17/types/data_device` 与 data-device 头文件

Wayland data-device 与 X11 DnD 共用的状态扩展。

已成为官方实现的回填：

| 功能 | fork 提交 | 官方提交 | 首个官方版本 | 升级动作 |
|---|---|---|---|---|
| drag focus surface 销毁监听 | `ded20be4f` | `35ab4533` | 0.18.2 | 0.18.2+ 删除 |
| touch drag 同样发出 motion signal | `bdeb7f60f` | `aaf82ee3` | 0.20.0 | 0.20+ 删除 |

fork 特有部分包括：`accepted`/`dnd_action` signal（`fe1794bd1`）、空 MIME 不算
accepted（`d60f8ec16`）、未 accepted 时的 drop 收尾（`278c7fc48`）、避免 drop
给源 client（`2f0c6656c`）、把 keyboard modifiers 发送给 drag focus
（`844f86166`, `bcf69a772`），以及等待 XdndStatus 时使用的位置缓存
（`cfdd7aa35`）。这些字段同时被 `src/xwayland` 使用，升级 data-device ABI 时需要一起
迁移，不能单独删除。

### `src/patches/wlroots/0.17/xwayland` 与 XWayland 头文件

XWM、selection 和 Xdnd 兼容层；这是当前 overlay 中与合成器耦合最强的一组。

已成为官方实现的回填：

| 功能 | fork 提交 | 官方提交 | 首个官方版本 |
|---|---|---|---|
| 公开 XWM XCB connection | `3bde70ec9` | `50eae512` | 0.18.0 |
| 修复 Xwayland server 内存泄漏 | `5de716f5f` | `2b8f94cf` | 0.18.1 |
| 监听 xwayland-shell destroy | `ff5bb89e9` | `09924224` | 0.18.1 |
| XWM destroy 时解除内部 DnD listener | `c60dbdbbb` | `b28b2691` | 0.18.2 |
| drag focus 生命周期修复 | `c496b3062`, `f7df4437d`, `82f70fed0` | `885bf8f5`, `1fc9409d`, `0c437f6e` | 0.18.2 |
| DnD 始终发送 finished | `3de8dc458` | `db2c907f` | 0.19.0 |
| 补全 `_NET_WM_STATE` | `6a86c630c` | `41e23318` | 0.19.0 |
| SIGCHLD 不导致 Xwayland 启动失败 | `a95dd1abf` | `631e5be0` | 0.19.0 |

fork 特有部分及来源：

- XdndProxy 和 openKylin DnD 状态机：`27c14d635`, `dd75d203d`, `cfdd7aa35`。
- override-redirect focus、重父窗口、map-request 属性读取与映射后 focus：
  `730364264`, `8f7c52a5d`, `65ee3e47e`, `545c2fec4`, `bfedfa787`。
- `set_size_hints`、Motif functions、modal 状态事件：`d8b8bc8c1`, `b36c028f2`,
  `4e8f2bd47`。
- 只对支持 ping 的 client 发 ping：`585c60f85`。
- 未聚焦 Xwayland surface 时允许读取 clipboard，以及 selection type 兼容：
  `a960d1ddb`, `d8b27ae8d`。
- `xwm_send_event_with_size` 返回值清理：`b2eaac5f4`。

升级到 0.19+ 时，以新版上游文件为起点，再移植上述仍为 fork 特有的逻辑；不要整份
沿用 0.17 overlay 文件。

### explicit-sync 与 DRM syncobj overlay 源码

来源为 fork 提交 `575bdc999`，把 `wp_linux_drm_syncobj_v1`、DRM syncobj timeline
和 timeline-point merger 从较新 wlroots 回填到 0.17。官方协议和基础 timeline API
自 0.18.0 起提供；wlcom 使用的 release helper 在 0.19.0 中提供；merger 在 0.20.0
中提供。因此只有升级到 0.20+、并核对 `src/server.c`、`src/scene/buffer.c` 和
`src/scene/surface.c` 的 API 后，才能整体删除这组 overlay 文件。

上游关键提交：

- `213bd88b`：`linux-drm-syncobj-v1` protocol implementation（0.18.0）。
- `7fc00ef7`：`wlr_drm_syncobj_timeline`（0.18.0）。
- `850dd7a7`：buffer release signal helper（0.19.0）。
- `288ba9e7`：timeline point merger（0.20.0）。

## 升级流程

1. 将新的官方 wlroots 源码导入 `libs/wlroots`，更新 `libs/README.md` 和
   `libs/README.zh.md` 的 tag/commit。
2. 以新版本官方文件为基础，按本账本的“首个官方版本”排除已经上游化的逻辑；不要
   直接复制旧版本的完整 overlay 文件。
3. 对仍为 fork 特有的条目重新审查新版实现，保留、改写或用硬件/互操作测试证明可删。
4. 在 `src/patches/wlroots/<版本>` 更新确有必要的兼容源码，并同步本文件；任何新回填
   都补齐来源与删除条件。
5. 从空构建目录执行 CMake configure、完整编译和测试，并至少覆盖 DRM multi-GPU、
   VT 切换、X11/Wayland 双向 DnD、clipboard、NET_WM_STATE 与 explicit-sync。

## 完整性检查

需要确认 Vendor 是否仍是纯官方树时，可把官方 tag 导出到临时目录，然后执行：

```sh
diff -qr libs/wlroots /tmp/wlroots-0.17.4
```

结果应为空。构建目录中的 `_deps/wlroots-src` 则应等价于历史
`open-kylin-wlroots@950dbeb6` 的运行时代码（不含 fork README 和 Debian 包装文件）。
