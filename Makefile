all: ymirc surtrc

ymirc:
	$(MAKE) -C ymirc

surtrc:
	$(MAKE) -C surtrc

run: all
	$(MAKE) -C surtrc run

test:
	$(MAKE) -C ymirc test

clean:
	rm -rf build

.PHONY: all ymirc surtrc run test clean
