# Maintainer: CharOfString <root@charofstring.cc>

pkgname=flakewm
pkgver=2.1.1.gxde8
pkgrel=1
pkgdesc='GXDE Wayland compositor'
arch=('x86_64' 'aarch64')
url='https://github.com/GXDE-OS/open-kylin-wlcom'
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

source=("$pkgname::git+$url.git")
sha256sums=('SKIP')

build() {
  cmake -S "$pkgname" -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=None \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DWLCOM_EXAMPLES=OFF \
    -DWLCOM_UKUI_THEME=ON \
    -DWLCOM_WLROOTS_RENDERERS=gles2,vulkan
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
