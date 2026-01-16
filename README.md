# nspire-fm

A powerful, light-weight file manager for the TI-Nspire series, designed for use with [Ndless](https://github.com/ndless-nspire/Ndless).

## Features

- **File Operations**: Browse, rename, and manage files on your handheld.
- **Integrated Viewer/Editor**: View and edit text files directly on device.
- **Fast & Efficient**: Optimized for the ARM-based Nspire hardware.
- **Clean UI**: Minimalist interface focused on functionality.

## Build Instructions

### Prerequisites

To build `nspire-fm`, you need the **Ndless SDK** installed and properly configured in your environment.

### Compiling

1.  Export the Ndless toolchain paths to your `PATH`:
    ```bash
    export PATH=/path/to/Ndless/ndless-sdk/bin:/path/to/Ndless/ndless-sdk/toolchain/install/bin:$PATH
    ```
    *(Replace `/path/to/Ndless` with your actual Ndless SDK location).*

2.  Run the makefile:
    ```bash
    make
    ```

3.  The build will produce:
    - `nspire-fm.elf`: The ARM executable binary.
    - `nspire-fm.tns`: The final executable to be transferred to the calculator.

## Installation

1.  Transfer `nspire-fm.tns` to your TI-Nspire using the TI Computer Software or a compatible transfer tool.
2.  Ensure your handheld is running Ndless.
3.  Open `nspire-fm.tns` from the document browser.

## License

This project is licensed under the **GNU General Public License v3.0**. See the [LICENSE](LICENSE) file for details.

## Credits

- Developed for the TI-Nspire community.
- Built powered by the [Ndless SDK](https://github.com/ndless-nspire/Ndless).
