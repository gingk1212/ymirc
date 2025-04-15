.PHONY: all ymirc surtrc run clean

all: ymirc surtrc

ymirc:
	$(MAKE) -C ymirc

surtrc:
	$(MAKE) -C surtrc

run:
	$(MAKE) -C surtrc run

clean:
	rm -rf build
