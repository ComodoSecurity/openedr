
all: debian9 debian10 ubuntu1604 ubuntu1804 arch fedora centos

debian9: Debian9.Dockerfile
	docker build --rm -f $< ..

debian10: Debian10.Dockerfile
	docker build --rm -f $< ..

ubuntu1604: Ubuntu1604.Dockerfile
	docker build --rm -f $< ..

ubuntu1804: Ubuntu1804.Dockerfile
	docker build --rm -f $< ..

arch: ArchLinux.Dockerfile
	docker build --rm -f $< ..

fedora: Fedora.Dockerfile
	docker build --rm -f $< ..

osx:
	export OS=osx
	cd .. && ./docker/build_test_install.sh
