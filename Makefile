MAKEFLAGS += --no-print-directory

all: ymirc surtrc

ymirc:
	@echo "Building ymirc..."
	@$(MAKE) -C ymirc

surtrc:
	@echo "Building surtrc..."
	@$(MAKE) -C surtrc

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

.PHONY: all ymirc surtrc run test clean
