## BEElink

<p align=center><img src="https://hackster.imgix.net/uploads/attachments/1242449/_SYVVWyeWfc.blob?auto=compress%2Cformat&w=450&h=337&fit=min"/></p>

#### What is BEElink ?

BEElink is an IoT device designed to connect bee hives. It allows beekeepers to monitor their bee hives in real time.

BEElink connects data from a multitude of sensors and send them over the Sigfox network. The data is received and displayed on a friendly web dashboard (Ubidots).

Software and hardare are completely open source ! You can check this [hackster article](https://www.hackster.io/beelink/beelink-5db0c4) for a guide to create a BEElink device.

This repository provides all the source code used to program BEElink, with mbed-os on a STM32L432KC microcontroler.

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
