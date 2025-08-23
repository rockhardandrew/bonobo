BONOBO_SRC = main.c metadata.c files.c
MD4C_SRC = third-party/md4c-html.c third-party/md4c.c third-party/entity.c
# this provides cpu specific optimizations. Avoid if you're running this binary on multiple machines
bonobo:
	clang -static -O2 -march=native ${BONOBO_SRC} ${MD4C_SRC} -o bonobo
	strip bonobo

# debug build - useful for detecting memory safety issues and other C footguns
debug:
	clang -fsanitize=address -O1 -fno-omit-frame-pointer -g ${BONOBO_SRC} ${MD4C_SRC} -o bonobo

# run this before you run git commit
clean:
	rm bonobo

# compiles with tcc for faster build speeds. This is useful for CI builds or for quick tests (my laptop too slow for clang)
fast:
	tcc ${BONOBO_SRC} ${MD4C_SRC} -o bonobo

# installs binary on linux, needs write priviledges to /usr/local/bin (in other words run as root)
install:
	install -m 755 bonobo /usr/local/bin/bonobo
