.PHONY: all build test clean lint tidy format coverage run help

BUILD_DIR ?= build
CMAKE ?= cmake

UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME_S),Windows)
    NPROC ?= $(NUMBER_OF_PROCESSORS)
else
    NPROC ?= $(shell nproc 2>/dev/null || echo 4)
endif

all: build

build:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
	cd $(BUILD_DIR) && $(CMAKE) --build . --parallel $(NPROC)

release:
	@mkdir -p $(BUILD_DIR)-release
	cd $(BUILD_DIR)-release && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF -DBUILD_TESTS=ON
	cd $(BUILD_DIR)-release && $(CMAKE) --build . --parallel $(NPROC)

test: build
	./$(BUILD_DIR)/test_all

clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR)-release

lint:
	cd $(BUILD_DIR) && $(CMAKE) --build . --parallel $(NPROC) 2>&1 | head -100

tidy:
	find src/ -name '*.cpp' | xargs clang-tidy -p $(BUILD_DIR)/ --quiet 2>/dev/null || true

cppcheck:
	cppcheck --enable=all --suppress=missingIncludeSystem --inconclusive \
		--std=c++17 -I src/core -I src/ai -I src/network -I src/ui \
		-I src/utils -I src/engine src/ 2>/dev/null || true

format:
	find src/ -name '*.cpp' -o -name '*.h' | xargs clang-format -i -style=file 2>/dev/null || true

coverage: build
	./$(BUILD_DIR)/test_all
	lcov --capture --directory . --output-file coverage.info --rc lcov_branch_coverage=1 2>/dev/null || true
	lcov --remove coverage.info '/usr/*' '*/raylib-*' '*/tests/*' --output-file coverage.info 2>/dev/null || true
	genhtml coverage.info --output-directory coverage_report 2>/dev/null || true
	@echo "Report: coverage_report/index.html"

run: build
	./$(BUILD_DIR)/gameuno

help:
	@echo "Targets:"
	@echo "  make build     - Configure and build (debug)"
	@echo "  make release   - Build with optimizations"
	@echo "  make test      - Run unit tests"
	@echo "  make run       - Launch the game"
	@echo "  make lint      - Check compiler warnings"
	@echo "  make tidy      - Run clang-tidy"
	@echo "  make cppcheck  - Run cppcheck"
	@echo "  make format    - Auto-format source files"
	@echo "  make coverage  - Generate coverage report"
	@echo "  make clean     - Remove build artifacts"
