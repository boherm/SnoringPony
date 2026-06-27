# RTL-SDR vendored libraries

The RF Coordination panel can scan the real RF environment with an RTL2832U
(RTL-SDR) USB dongle. This requires **librtlsdr** and its dependency **libusb-1.0**,
prebuilt per platform and dropped here.

When the expected library for the current platform is present, CMake links it and
defines `SP_HAS_RTLSDR=1` (hardware scanning enabled). Otherwise the app still builds
and runs with the simulated source only (`SP_HAS_RTLSDR=0`).

## Expected layout

```
external/rtlsdr/
├── include/
│   └── rtl-sdr.h            (+ rtl-sdr_export.h if your build splits it)
└── lib/
    ├── osx/
    │   ├── arm64/           librtlsdr.dylib   libusb-1.0.0.dylib
    │   └── x86_64/          librtlsdr.dylib   libusb-1.0.0.dylib
    ├── win/
    │   └── x64/             rtlsdr.lib  rtlsdr.dll  (+ libusb-1.0.dll if separate)
    └── linux/
        └── x86_64/          librtlsdr.so   (or librtlsdr.a)
```

CMake decides `SP_HAS_RTLSDR` by checking for the rtlsdr library in the platform
folder (e.g. `lib/osx/arm64/librtlsdr.dylib`, `lib/win/x64/rtlsdr.lib`,
`lib/linux/x86_64/librtlsdr.{so,a}`).

## Per-platform notes

### macOS

`brew install librtlsdr`, then copy the dylibs + `rtl-sdr.h`.

**Gotcha**: raw Homebrew bottle dylibs keep an unrelocated install name
(`@@HOMEBREW_PREFIX@@/...` on Apple Silicon, `/usr/local/opt/...` on Intel) that
dyld cannot resolve at runtime (`Symbol not found` / `Library not loaded`). Rewrite
them to `@rpath` and re-sign — CMake adds an rpath to the vendored folder. For
`lib/osx/arm64/` (adapt the libusb source path / arch as needed):

```bash
cd external/rtlsdr/lib/osx/arm64
chmod u+w librtlsdr.dylib libusb-1.0.0.dylib
install_name_tool -id @rpath/librtlsdr.dylib librtlsdr.dylib
install_name_tool -change @@HOMEBREW_PREFIX@@/opt/libusb/lib/libusb-1.0.0.dylib \
                          @rpath/libusb-1.0.0.dylib librtlsdr.dylib
install_name_tool -id @rpath/libusb-1.0.0.dylib libusb-1.0.0.dylib
codesign --force --sign - libusb-1.0.0.dylib librtlsdr.dylib
```

Verify with `otool -L librtlsdr.dylib` (all deps `@rpath/...`). File names must match
the ids. For distribution, bundle these into the .app and use `@loader_path` rpaths.

### Windows

Prebuilt binaries are published by the rtl-sdr project / osmocom (e.g. the
`rtl-sdr-w64` release). MSVC links against an **import library (`rtlsdr.lib`)**, not
the DLL. If you only have `rtlsdr.dll`, generate the import lib once (x64 Developer
Command Prompt):

```bat
dumpbin /exports rtlsdr.dll
:: put the exported function names under "EXPORTS" in rtlsdr.def, then:
lib /def:rtlsdr.def /machine:x64 /out:rtlsdr.lib
```

(Or with MSYS2/MinGW: `gendef rtlsdr.dll` then `dlltool -d rtlsdr.def -l rtlsdr.lib`.)
Drop `rtlsdr.lib` + `rtlsdr.dll` (and `libusb-1.0.dll` if the build keeps libusb
separate) into `lib/win/x64/`. CMake copies the DLL(s) next to the built `.exe`.

**From macOS / Linux (no Windows needed)** — generate the import lib with LLVM
(`brew install llvm`, then `export PATH="/opt/homebrew/opt/llvm/bin:$PATH"`):

```bash
cd external/rtlsdr/lib/win/x64      # rtlsdr.dll here
{ echo "LIBRARY rtlsdr.dll"; echo "EXPORTS"; \
  llvm-readobj --coff-exports rtlsdr.dll | grep -oE 'rtlsdr_[A-Za-z0-9_]+' | sort -u; } > rtlsdr.def
llvm-lib /def:rtlsdr.def /machine:x64 /out:rtlsdr.lib
# check whether libusb is a separate DLL dependency:
llvm-objdump -p rtlsdr.dll | grep -i '\.dll'
```

### Linux

`apt install librtlsdr-dev libusb-1.0-0-dev`, then copy `librtlsdr.so` (or the static
`librtlsdr.a`) into `lib/linux/x86_64/`. A static `.a` does not contain libusb, so the
build also links the system `libusb-1.0` + `pthread`/`m`.

## License

Shipping these libraries with the app must comply with their licenses (librtlsdr is
GPL-2.0, libusb is LGPL-2.1). Keep the source/offer accordingly.
