# Maintainer: ms <ms@kilabit.info>
pkgname=rescached-git
pkgver=r55.57b1070
pkgrel=1
pkgdesc="Resolver/DNS cache daemon"
arch=('i686' 'x86_64')
url="http://kilabit.info"
license=('custom:BSD')
makedepends=('git')
source=(rescached-git::git+https://github.com/shuLhan/rescached.git)
sha1sums=('SKIP')

pkgver() {
	cd "$pkgname"
	printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
	cd "$srcdir/$pkgname/src"
	echo `pwd`
	echo ">>"
	echo ">> updating ..."
	echo ">>"
	git submodule update --init
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
	cd "$srcdir/$pkgname/src"

	make DESTDIR="$pkgdir/" install
}
