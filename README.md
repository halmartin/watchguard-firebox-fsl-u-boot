# Contents

This repository contains the bootloader source code for the WatchGuard:
* Firebox T20 (LS1023A @ 1.0GHz)
* Firebox T25 (LS1043A @ 1.0GHz)
* Firebox T40 (LS1043A @ 1.0GHz)
* Firebox T45 (LS1043A @ 1.6GHz)
* Firebox T80 (LS1046A @ 1.2GHz)
* Firebox T85 (LS1046A @ 1.8GHz)

# Requesting source code

Source code was requested via WatchGuard support. They require proof of ownership of the hardware, a photo of the device showing the serial number is sufficient.

# Building

WatchGuard ONLY provides the source archive and U-Boot configuration files for the above devices. Build scripts to produce a flashable image were not provided.

As a convenience to the user, the author provides a containerised (docker/podman) method to build the U-boot source provided by WatchGuard.

To build:
```
make
```

This will build the Ubuntu-based container and then compile U-Boot.
