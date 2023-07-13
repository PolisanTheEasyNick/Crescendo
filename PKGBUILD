# Maintainer: ob3r0n <ob3r0n@polisan.ddns.net>
pkgname=crescendo-git
pkgver=bc04330
pkgrel=1
pkgdesc="Mediaplayer and Controller"
arch=('x86_64')
#source=("https://github.com/PolisanTheEasyNick/Crescendo")
source=("git+https://github.com/PolisanTheEasyNick/Crescendo.git")
license=('MIT')
depends=('git' 'pugixml' 'gtkmm-4.0')
optdepends=('pulseaudio: Changing sound device'
           'sdl2: Playing local files'
           'sdl2_mixer: Playing local files'
           'taglib: Reading metadata of local files'
           'dbus: Controlling local supported players'
           'sdbus-cpp: Controlling local supported players')
conflicts=('crescendo')
provides=('crescendo')

pkgver() {
    cd "$srcdir/Crescendo"
    git describe --long --tags --always | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
    cd "$srcdir/Crescendo"
    # Create build directory
    mkdir build
    cd build
    # Configure the project
    cmake ..
    # Build the project
    make
}

package() {
    cd "$srcdir/Crescendo/build"
    make DESTDIR="$pkgdir/" install
}
sha256sums=('SKIP')
