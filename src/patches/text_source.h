// SPDX-License-Identifier: GPL-1.0-or-later
#ifndef WLCOM_TEXT_SOURCE_H
#define WLCOM_TEXT_SOURCE_H
struct wlr_data_source;
/* Provide an explicit UTF-8 alias for Wayland text/plain drag sources. */
void ky_text_source_add_utf8(struct wlr_data_source *source);
#endif
