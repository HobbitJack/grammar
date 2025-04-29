# GRAMMAR
`grammar` is a grammar-checking command-line program using the `harper` grammar-checking library.
As support for the C interface is currently not yet merged into upstream, you will need to use the `how-to-use-harper-from-c` branch at https://github.com/hippietrail/harper/.

## INSTALLATION
Installation is currently semi-manual.
After compiling `libharper_c.so`, you'll need to move that to some place your compiler/linker can find it (I symlinked it to `/usr/lib/libharper_c.so`).
Then, build and installe th program with
```
$ make
$ make docs
$ make install
$ make clean
```

Uninstallation can be done with
```
$ make uninstall
```

An alternative installation process for development is
```
$ make install-symlink
```
which will install to the destination the program and gzipped man page as symlinks.
