What is Kiirocoin?
--------------

[Kiirocoin](https://kiirocoin.org) is a privacy focused cryptocurrency that utilizes zero-knowledge proofs which allows users to destroy coins and then redeem them later for brand new ones with no transaction history.

Kiirocoin also utilises [Dandelion++](https://arxiv.org/abs/1805.11060) to obscure the originating IP of transactions without relying on any external services such as Tor/i2P.

Kiirocoin uses KiiroPoW (a ProgPoW variant) as its Proof-of-Work GPU focused algorithm which is FPGA/ASIC resistant.


Linux Build Instructions and Notes
==================================

Kiirocoin contains build scripts for its dependencies to ensure all component versions are compatible. For additional options
such as cross compilation, read the [depends instructions](depends/README.md)

Alternatively, you can build dependencies manually. See the full [unix build instructions](doc/build-unix.md).

Bootstrappable builds can [be achieved with Guix.](contrib/guix/README.md)

Development Dependencies (compiler and build tools)
----------------------

- Debian/Ubuntu/Mint (minimum Ubuntu 18.04):

    ```
    sudo apt-get update
    sudo apt-get install git curl python3 build-essential libtool automake pkg-config cmake
    # Also needed for GUI wallet only:
    sudo apt-get install qttools5-dev qttools5-dev-tools libxcb-xkb-dev bison
    ```

- Redhat/Fedora:

    ```
    sudo dnf update
    sudo dnf install bzip2 perl-lib perl-FindBin gcc-c++ libtool make autoconf automake cmake patch which
    # Also needed for GUI wallet only:
    sudo dnf install qt5-qttools-devel qt5-qtbase-devel xz bison
    sudo ln /usr/bin/bison /usr/bin/yacc
    ```
- Arch:

    ```
    sudo pacman -Sy
    sudo pacman -S git base-devel python cmake
    ```

Build Kiirocoin
----------------------

1.  Download the source:

        git clone https://github.com/Kiirocoin/kiiro
        cd kiiro

2.  Build dependencies and kiirocoin:

    Headless (command-line only for servers etc.):

        cd depends
        NO_QT=true make -j`nproc`
        cd ..
        ./autogen.sh
        ./configure --prefix=`pwd`/depends/`depends/config.guess` --without-gui
        make -j`nproc`

    Or with GUI wallet as well:

        cd depends
        make -j`nproc`
        cd ..
        ./autogen.sh
        ./configure --prefix=`pwd`/depends/`depends/config.guess`
        make -j`nproc`

3.  *(optional)* It is recommended to build and run the unit tests:

        ./configure --prefix=`pwd`/depends/`depends/config.guess` --enable-tests
        make check


macOS Build Instructions and Notes
=====================================
See (doc/build-macos.md) for instructions on building on macOS.



Windows (64/32 bit) Build Instructions and Notes
=====================================
See (doc/build-windows.md) for instructions on building on Windows 64/32 bit.

## Contributors

### Code Contributors

 Code creator is https://firo.org "firo" team, thank you.

