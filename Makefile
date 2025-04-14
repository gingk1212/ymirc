.PHONY: all surtrc clean run

all: surtrc

surtrc:
	$(MAKE) -C surtrc

run:
	$(MAKE) -C surtrc run

clean:
	rm -rf build
