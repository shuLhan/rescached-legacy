# Maintainer: ms <ms@kilabit.info>
_pkgname=rescached
pkgname=rescached-git
pkgver=r55.57b1070
pkgrel=1
pkgdesc="Resolver/DNS cache daemon"
arch=('i686' 'x86_64')
url="http://kilabit.info/projects/"
license=('custom:BSD')
makedepends=('git')
source=("$_pkgname::git+https://github.com/shuLhan/rescached.git")
sha1sums=('SKIP')

pkgver() {
	cd "$srcdir/$_pkgname/src"
	printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

prepare() {
	cd "$srcdir/$_pkgname/src"
	git submodule init
	git submodule update
}

build() {
	cd "$srcdir/$_pkgname/src"
	echo ">>"
	echo ">> cleaning ..."
	echo ">>"
	make distclean
	echo ">>"
	echo ">> make ..."
	echo ">>"
	unset CXXFLAGS
	make || return 1
}

package() {
	cd "$srcdir/$_pkgname/src"

	make DESTDIR="$pkgdir/" install
}
