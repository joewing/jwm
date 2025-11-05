# SBWM - SIBOS Window Manager

  SBWM is a lightweight window manager for the SIBOS operating system, forked from [JWM (Joe's Window Manager)](https://github.com/joewing/jwm).

  ## Version
  v0.1.0 - Initial fork based on JWM 2.4.6

  ## Upstream Project
  - **Original Author**: Joe Wingbermuehle
  - **Original Project**: https://joewing.net/projects/jwm/
  - **Source Repository**: https://github.com/joewing/jwm

  ## Changes from JWM
  - Renamed binary: `jwm` → `SBWM`
  - Version changed to 0.1.0 for independent versioning
  - Planned features:
    - Pre-rendered transparency with blur effects (no compositor needed)
    - SIBOS-specific system integrations
    - Performance optimizations for image-based OS

  ## Configuration
  SBWM currently uses `.jwmrc` configuration files (same format as JWM 2.4.6).

  ## License
  MIT License (same as upstream JWM)

  See LICENSE file for full copyright information.

  ## Building

  ./configure --prefix=/usr --sysconfdir=/etc
  make
  make install 

  For SIBOS

  SBWM is designed for the SIBOS operating system. Build scripts are available in the SIBOS source tree at packages/build_sbwm.sh.
