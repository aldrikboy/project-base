MAKEFLAGS += -s
MAKEFLAGS += --always-make

main_file = src/main.c
output_file = libprojectbase
flags = -m64 -c -Wall -DMODE_DEBUG -g
release_flags = -m64 -c -Wall -O3

gcc_install_dir := $(shell which "gcc")
gcc_install_dir := $(subst gcc,,$(gcc_install_dir))

ifeq ($(OS), Windows_NT)
	install_dir = $(gcc_install_dir)../x86_64-w64-mingw32/
	permissions = 
	libs = 

	# Commands
	install_deps_command = empty
	install_lib_command = install_windows
	create_examples_command = examples_windows
	create_tests_command = tests_windows
	generate_docs_command = docs_windows
else
	install_dir = /usr/
	permissions = sudo
	libs = -lX11 -lm -ldl
	# -Wl,-Bstatic -ldl -Wl,-Bdynamic

	# Commands
	install_deps_command = install_deps
	install_lib_command = install_linux
	create_examples_command = examples_linux
	create_tests_command = tests_linux
	generate_docs_command = docs_linux
endif

include_dir = $(install_dir)include/projectbase
lib_dir = $(install_dir)lib/$(output_file)


all:
	make $(install_deps_command)
	make build
	make examples

empty:
	@$(NOECHO) $(NOOP)

# Install deps (Linux)
install_deps:
	sudo apt-get --yes --force-yes install freeglut3-dev
	sudo apt-get --yes --force-yes install binutils-gold
	sudo apt-get --yes --force-yes install g++
	sudo apt-get --yes --force-yes install mesa-common-dev
	sudo apt-get --yes --force-yes install build-essential
	sudo apt-get --yes --force-yes install libglew1.5-dev
	sudo apt-get --yes --force-yes install libglm-dev
	sudo apt-get --yes --force-yes install mesa-utils
	sudo apt-get --yes --force-yes install libglu1-mesa-dev
	sudo apt-get --yes --force-yes install libgl1-mesa-dev
	sudo apt-get --yes --force-yes install libxrandr-dev

# Build (Windows + Linux)
build:
	$(permissions) mkdir -p "build/"
	$(permissions) rm -rf "$(include_dir)"
	$(permissions) mkdir -p "$(include_dir)"

	$(permissions) ld -r -b binary -o build/data.o src/resources/mono.ttf
	$(permissions) gcc $(flags) $(main_file) -o build/$(output_file).o $(libs)
	$(permissions) ar rcs build/$(output_file).a build/$(output_file).o build/data.o

	#$(permissions) gcc $(flags) src/addons/c_parser.c -o build/$(output_file)-parser.a

	make $(install_lib_command)

install_windows:
	$(permissions) cp -a "src/." "$(include_dir)" 2>/dev/null || :
	$(permissions) cp "build/$(output_file).a" "$(lib_dir).a" 2>/dev/null || :
	$(permissions) cp "build/$(output_file)-parser.a" "$(lib_dir)-parser.a" 2>/dev/null || :

	# github action shite
	$(permissions) mkdir -p "C:/ProgramData/Chocolatey/lib/mingw/tools/install/mingw64/x86_64-w64-mingw32/include/projectbase" 2>/dev/null || :
	$(permissions) cp -a "src/." "C:/ProgramData/Chocolatey/lib/mingw/tools/install/mingw64/x86_64-w64-mingw32/include/projectbase" 2>/dev/null || :
	$(permissions) cp "build/$(output_file).a" "C:/ProgramData/Chocolatey/lib/mingw/tools/install/mingw64/x86_64-w64-mingw32/lib/$(output_file).a" 2>/dev/null || :
	$(permissions) cp "build/$(output_file)-parser.a" "C:/ProgramData/Chocolatey/lib/mingw/tools/install/mingw64/x86_64-w64-mingw32/lib/$(output_file)-parser.a" 2>/dev/null || :

install_linux:
	$(permissions) cp -a "src/." "$(include_dir)" 2>/dev/null || :
	$(permissions) cp "build/$(output_file).a" "$(lib_dir).a" 2>/dev/null || :
	$(permissions) cp "build/$(output_file)-parser.a" "$(lib_dir)-parser.a" 2>/dev/null || :

## Tests (Windows + Linux)
tests:
	make build
	make $(create_tests_command)

tests_windows:
	gcc -m64 -g tests/main.c -o build/tests.exe -lprojectbase -lprojectbase-parser $(libs)
	./build/tests

tests_linux:
	$(permissions) gcc -m64 -g tests/main.c -o build/tests -lprojectbase -lprojectbase-parser $(libs)
	$(permissions) sudo chmod +x build/tests
	$(permissions) ./build/tests

## Examples (Windows + Linux)
examples:
	$(permissions) ld -r -b binary -o build/data.o examples/en.mo
	make $(create_examples_command)

examples_windows:
	$(permissions) gcc -m64 -g -DMODE_DEBUG examples/example_window.c build/data.o -o build/example_window.exe -lprojectbase $(libs)

examples_linux:
	$(permissions) gcc -m64 -g -DMODE_DEBUG examples/example_window.c build/data.o -o build/example_window -lprojectbase $(libs)
	$(permissions) chmod +x build/example_window

docs:
	$(permissions) gcc -m64 -g docs/gen_docs.c -o build/gen_docs.exe -lprojectbase $(libs)
	$(permissions) ./build/gen_docs
	## $(permissions) pandoc build/docs.txt -o build/docs.pdf

cloc:
	cloc-1.88.exe --exclude-dir=external src/