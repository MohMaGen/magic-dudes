> [!WARNING]
> migrated to [codeberg](https://codeberg.org/mohmagen/magic-dudes.git)


# MAGIC DUDES

Game about magic dudes, where you can create your own magic spells and
explore the butiful world of magic and author's ill imagination.

## BUILD

This project use [nob.h](https://github.com/tsoding/nob.h) (nobuild) library
to create it's own build system. 

### Quick start:

To just build everything with default options just build the build system
and run it.
```console
cc -o nob ./nob.c
./nob build all
```

### Build and execute demos

The project consist of game engine library build with raylib and with multiple
demos.

To build specific demo run:
```console
./nob build <demo-name>
```

To execute specific demo run:
```console
./nob exec <demo-name>
```
