// SPDX-License-Identifier: GPL-1.0-or-later
#ifndef KY_PATCHES_LINUX_DMABUF_H
#define KY_PATCHES_LINUX_DMABUF_H
#include <stdbool.h>
struct wlr_linux_dmabuf_v1;
/* Install the historical vmwgfx GEM-close workaround through the public API. */
bool ky_linux_dmabuf_init_vmware_check(struct wlr_linux_dmabuf_v1 *dmabuf, int renderer_fd);
#endif
