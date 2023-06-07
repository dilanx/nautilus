# VNC GPU Information




STUFF BELOW IS OUTDATED UPDATE FOR LATEST STUFF

- new libvnc stuff
- conflicting types
- add instructions on using lwip

[libvnc](https://libvnc.github.io) is installed in `/libvnc`.

## Preparing `libvnc`

`cmake` and `pkg-config` are required.

```
cd libvnc
mkdir build
cd build
cmake ..
cmake --build .
make install
```

## Notes on changes

### Incorporating `libvnc`

We installed `libvnc` into the `libvnc` directory. Then, added the following to `src/dev/Makefile`:

```
CFLAGS += $(pkg-config --cflags libvncserver)
LDFLAGS += $(pkg-config --libs libvncserver)
```

We ran into some conflicting types between Nautilus and the C standard library, which we resolved:

```diff
// include/nautilus/naut_types.h

- typedef unsigned long off_t
+ typedef long int off_t

// include/nautilus/libccompat.h

- typedef uint64_t off_t
+ typedef long int off_t
```
