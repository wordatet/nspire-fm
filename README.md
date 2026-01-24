# nspire-fm

A powerful, light-weight file manager for the TI-Nspire series, designed for use with [Ndless](https://github.com/ndless-nspire/Ndless).

## Features

- **File Operations**: Browse, copy, cut, paste, rename, and delete files.
- **Integrated Viewer/Editor**: View and edit text files directly on device.
- **Image Viewer**: Display PNG, JPG, BMP, and TGA images (uses [stb_image](https://github.com/nothings/stb)).
- **Hex Viewer**: Inspect binary files.
- **Fast & Efficient**: Optimized for the ARM-based Nspire hardware.
- **Clean UI**: Minimalist interface focused on functionality.

## Build Instructions

### Prerequisites

- **Ndless SDK** installed and built ([Installation Guide](https://github.com/ndless-nspire/Ndless))
- **nspire-io** library built (included in Ndless SDK thirdparty)

### Environment Setup

Set the `NDLESS_SDK` environment variable to your Ndless SDK path:

```bash
# Add to your ~/.bashrc or ~/.zshrc
export NDLESS_SDK="/path/to/Ndless/ndless-sdk"
```

Then reload your shell:
```bash
source ~/.bashrc
```

### Compiling

```bash
make
```

The build will produce:
- `nspire-fm.elf`: The ARM executable binary.
- `nspire-fm.tns`: The final executable to transfer to the calculator.

## Installation

1.  Transfer `nspire-fm.tns` to your TI-Nspire using the TI Computer Software or a compatible transfer tool.
2.  Ensure your handheld is running Ndless.
3.  Open `nspire-fm.tns` from the document browser.

## License

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.

## Credits

- Developed for the TI-Nspire community.
- Built powered by the [Ndless SDK](https://github.com/ndless-nspire/Ndless).
