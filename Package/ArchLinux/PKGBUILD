# Maintainer: Rosalie Wanders <rosalie@mailbox.org>
_pkgname=rmg
pkgname=${_pkgname}-git
pkgver=git
pkgrel=1
pkgdesc="Rosalie's Mupen GUI"
arch=('x86_64')
url="https://github.com/Rosalie241/${_pkgname}"
license=('GPLv3')

provides=("$_pkgname")
conflicts=("$_pkgname")

depends=("hidapi" "libsamplerate" "minizip" "sdl2" "zlib" "freetype2" "qt6-base" "qt6-svg" "xdg-user-dirs")
makedepends=("git" "pkg-config" "nasm" "cmake" "cargo")

source=("git+https://github.com/Rosalie241/${_pkgname}.git")
sha256sums=('SKIP')

pkgver()
{
    cd "$_pkgname"

    git describe --tags --always | sed -e 's|^v||;s|-|.|g'
}

prepare()
{
    cd "$srcdir/${_pkgname}"

    mkdir -p build
}

build()
{
    cd "$srcdir/${_pkgname}/build"

    cmake -S "$srcdir/${_pkgname}" -B "$srcdir/${_pkgname}/build" \
                -DCMAKE_BUILD_TYPE="Release" \
                -DPORTABLE_INSTALL="OFF" \
                -DCMAKE_INSTALL_PREFIX="/usr" \
                -G "Unix Makefiles"
    make
}

package()
{
    cd "$srcdir/${_pkgname}/build"

    make install DESTDIR="$pkgdir"
}
