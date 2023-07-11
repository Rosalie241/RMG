# Maintainer: Rosalie Wanders <rosalie@mailbox.org>
pkgname=rmg
pkgver=0.4.3
pkgrel=1
pkgdesc="Rosalie's Mupen GUI"
arch=('x86_64')
url="https://github.com/Rosalie241/$pkgname"
license=('GPL3')

depends=("hidapi" "libsamplerate" "speexdsp" "minizip" "sdl2" "zlib" "freetype2" "qt6-base" "qt6-svg" "xdg-user-dirs")
makedepends=("git" "nasm" "cmake" "ninja")

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
                -DNO_RUST="ON" \
                -G "Ninja"

    cmake --build "$srcdir/$pkgname/build"
}

package()
{
    cmake --install "$srcdir/$pkgname/build" --prefix="$pkgdir/usr"
}
