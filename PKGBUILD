# Maintainer: CharOfString <root@charofstring.cc>

pkgname=flakewm
pkgver=3.0.0.gxde3
pkgrel=1
pkgdesc='GXDE Wayland compositor'
arch=('x86_64' 'aarch64')
url='https://github.com/flake-wm/flake-wm'
license=('GPL-1.0-or-later')
depends=(
  'cairo'
  'glib2'
  'json-c'
  'libdisplay-info'
  'libdrm'
  'libepoxy'
  'libglvnd'
  'libinput'
  'libjpeg-turbo'
  'libliftoff'
  'libpng'
  'librsvg'
  'libunwind'
  'libxcb'
  'libxkbcommon'
  'mesa'
  'openssl'
  'pango'
  'pixman'
  'seatd'
  'systemd-libs'
  'vulkan-icd-loader'
  'wayland'
  'xcb-util-errors'
  'xcb-util-renderutil'
  'xcb-util-wm'
  'xorg-xwayland'
)
makedepends=(
  'gettext'
  'git'
  'cmake'
  'glslang'
  'hwdata'
  'meson'
  'ninja'
  'pkgconf'
  'qt6-base'
  'qt6-declarative'
  'qt6-5compat'
  'systemd'
  'vulkan-headers'
  'wayland-protocols'
)

# CI supplies the checked-out commit so releases package the triggering source.
source=("$pkgname::${FLAKEWM_SOURCE:-git+$url.git}")
sha256sums=('SKIP')

build() {
  cmake -S "$pkgname" -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=None \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_BINDIR=bin \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DWLCOM_EXAMPLES=OFF \
    -DWLCOM_UKUI_THEME=ON \
    -DWLCOM_WLROOTS_RENDERERS=gles2,vulkan
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build || return 1

  # Refuse to publish a package missing the compositor or its session launcher.
  test -x "$pkgdir/usr/bin/flakewm" || return 1
  test -x "$pkgdir/usr/bin/startflakewm" || return 1
}
