## BEElink

#### Setup 

Install mbed CLI and gcc arm toolchain (you might need to configure the mbed_settings.py file).

Then, clone this repo and run :

```shell
mbed deploy
```

#### Build

```shell
mbed compile
```

You can use this command to flash your microcontroller and open a serial terminal :

```shell
mbed compile --flash --sterm
```
