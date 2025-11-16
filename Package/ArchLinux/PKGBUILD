# Maintainer: Rosalie Wanders <rosalie@mailbox.org>
pkgname=rmg
pkgver=0.8.8
pkgrel=1
pkgdesc="Rosalie's Mupen GUI"
arch=('x86_64' 'aarch64')
url="https://github.com/Rosalie241/$pkgname"
license=('GPL3')

depends=("libusb" "hidapi" "libsamplerate" "speexdsp" "minizip" "sdl3" "zlib" "freetype2" "qt6-base" "qt6-svg" "qt6-websockets")
makedepends=("git" "nasm" "cmake" "vulkan-headers")

source=("git+https://github.com/Rosalie241/$pkgname.git#tag=v$pkgver")
sha256sums=('SKIP')

prepare()
{
    mkdir -p "$srcdir/$pkgname/build"
}

build()
{
    cmake -S "$srcdir/$pkgname" -B "$srcdir/$pkgname/build" \
                -DCMAKE_BUILD_TYPE="Release" \
                -DPORTABLE_INSTALL="OFF" \
                -DCMAKE_INSTALL_PREFIX="/usr" \
                -G "Unix Makefiles"

    cmake --build "$srcdir/$pkgname/build"
}

package()
{
    cmake --install "$srcdir/$pkgname/build" --prefix="$pkgdir/usr"
}
