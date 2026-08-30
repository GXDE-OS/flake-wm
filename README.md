# FlakeWM
FlakeWM is a Wlroots-based Wayland compositor. This compositor is current developing.

## Dependencies
> The version info of vendored library could be found at (libs/README.md)[./libs/README.md].

### Core Requirements (VENDORED)
* Wlroots 0.20.2
* Waylib

### Dependencies
Okay, we fully realize that the versions of `libdrm`, `pixman`, and `wayland` required by Wlroots 0.20.2 may be too new for some distros. Hence we also vendored those three library and they will be statically linked once if system packages won't satisify the requirements.

But one shall also aware that vendoring those three library & statically linking them is generally NOT a good practice.

## Works Referenced
* GXDE Wayland Compositor
* LabWC
* Wayfire
* Treeland

## License
GPLv3+
