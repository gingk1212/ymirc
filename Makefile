MAKEFLAGS += --no-print-directory

all: ymirc surtrc ymircsh

ymirc:
	@echo "Building ymirc..."
	@$(MAKE) -C ymirc

surtrc:
	@echo "Building surtrc..."
	@$(MAKE) -C surtrc

ymircsh:
	@echo "Building ymircsh..."
	@$(MAKE) -C ymircsh

run: all
	@$(MAKE) -C surtrc run

test:
	@echo "Running tests..."
	@$(MAKE) -C ymirc test

clean:
	@echo "Cleaning up..."
	rm -rf build

bear:
	@echo "Generating compilation database..."
	bear -- make clean all test

install-linux:
	@echo "Install linux image and initramfs..."
	mkdir -p build/img
	cp examples/linux/bzImage examples/linux/rootfs.cpio.gz build/img

.PHONY: all ymirc surtrc ymircsh run test clean install-linux
