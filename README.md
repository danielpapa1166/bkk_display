<p align="center">
    <img src="doc/header_gif.gif" alt="Header Gif" width="500"/>
    <p align="center">Routes from "Diószegi út (Vérellátó szolgálat)" and "Hollókő utca" to the city center.</p>
</p>

# BKK Display

BKK Display is a Yocto-based project that uses a custom BKK API submodule to fetch real-time public transport data from the Budapest Public Transport Center’s key-accessed server. With a Qt-powered UI, it lets you display and configure arrival times for selected stations, offering a simple and personalized solution for commuting.

## Motivation

Living in Budapest, I have multiple bus stations near my home, each in opposite directions from my apartment. These stations provide me with options for getting into the city for work, socializing with friends, or running daily tasks. However, I often need a quick way to decide which station to use, depending on the buses’ real-time schedules. This little project was born out of this daily question, and it’s made life just a bit smoother.

<p align="center">
    <img src="doc/routes_to_the_city.png" alt="Bus Stations Map" width="500"/>
    <p align="center">A map illustrating the two lines and the stations I frequently use.</p>
</p>

Although, there is a feature in the official BudapestGO application to show nearby stations, but it displays every arrivals for every nearby stations, and I found it sometimes confusing and chaotic. It displays lines and stations you don't care about, but the BKK Display can be configured to meet your needs. 

<p align="center">
    <img src="doc/mobile_app_vs_display.png" alt="Mobile app vs display" width="500"/>
</p>

## Architecture

BKK Display is split into several small services and libraries, each responsible
for one part of the system. Yocto integrates these components into a custom
Raspberry Pi Linux image.

                  BKK API
                     │
                     ▼
                 bkk-api
                     │
                     ▼
              display services
          ┌──────────┼──────────┐
          ▼          ▼          ▼
     main-content  info-bar  screen-owner
          │          │          │
          └──────────┴──────────┘
                     │
                     ▼
                    Qt
                     │
                     ▼
                  Display

Configuration / supporting services:
    Web Setup Helper - Network Manager
    rbuflogd        - Logging
    bkk-tee         - Secure storage / OP-TEE


### BKK API

Interface responsible for retrieving real-time arrival information from the
Budapest public transport API. It hides the HTTP/API handling from the display
applications.

[More info →](https://github.com/danielpapa1166/bkk_api)

### Display subsystem

The UI is split into several processes with separate responsibilities:

- **bkk-screen-owner** - owns and coordinates the physical display - [detailed documentation →](meta-bkk-display/recipes-app/bkk-screen-owner/readme.md)
- **bkk-screen-main-content** - renders the real-time public transport information - [detailed documentation →](meta-bkk-display/recipes-app/bkk-screen-main-content/readme.md)
- **bkk-screen-info-bar** - renders auxiliary/status information - [detailed documentation →](meta-bkk-display/recipes-app/bkk-screen-info-bar/readme.md)

### Configuration and networking

The Network Manager hosts a wireless Access Point during the first boot to allow the user to configure a wifi connection. Once it is done, the Network Manager switches to the configured network putting the device on line. 

[Detailed documentation →](meta-bkk-display/recipes-app/network-manager/readme.md)

### Secure storage

`bkk-tee` integrates OP-TEE and provides secure storage for sensitive
configuration such as API credentials.

[Detailed documentation →](meta-bkk-display/recipes-app/bkk-tee/readme.md)

### Logging

`rbuflogd` provides centralized runtime logging for the different application
processes.

[Detailed documentation →](https://github.com/danielpapa1166/rbuflogd)

### Qt integration

The `meta-Qt` Yocto layer contains all Qt-specific build configuration. Qt is
configured for direct embedded rendering using EGLFS/KMS/GBM/LinuxFB without
requiring X11.

[Detailed documentation →](meta-Qt/README.md)

### Yocto integration

`meta-bkk-display` contains the application recipes, image configuration,
kernel/platform customization and the integration required to build the final
Raspberry Pi image.

`meta-platform-config` contains reusable platform configuration shared with
other Yocto projects.


## Getting started

### BOM 
- Raspberry Pi 4 
- [Display module](https://www.hestore.hu/prod_10046543.html?gross_price_view=1&source=gads&lang=hu&gad_source=1&gad_campaignid=17335669125&gbraid=0AAAAADz0y9LVUEhlznxppAJWXKHilyrkG&gclid=Cj0KCQjwk5nVBhDiARIsAHNGqadvKVvL6vDfL0_QVgMGo0ljST20p6xY2t_oxsik3DbWBf1SVzOLGsEaAvIQEALw_wcB)
- SD card 

## Build and flash

The project is built as a complete embedded Linux image using the **Yocto Project**. It extends Yocto's `core-image-full-cmdline` image with the BKK Display applications, services, and platform configuration.

After setting up the Yocto build environment, build the image with:

```bash
source poky/oe-init-build-env
bitbake core-image-full-cmdline
```

The generated `.wic` image can be found under:

```text
tmp/deploy/images/<machine>/
```

Flash the image to an SD card using a tool such as **Raspberry Pi Imager**, `bmaptool`, or `dd`. Insert the SD card into the Raspberry Pi and boot the device; the required BKK Display services are included in the image and started automatically.


### User config 
#### Network Configuration 
For the first time the device hosts an Access Point with that you can set your wifi credentials at `192.168.4.1:8080`. 

To streamline the user experience, the Display shows QR codes for connecting to the access point and opening the service point. The QR code is generated during runtime using [this module](https://github.com/danielpapa1166/qr_code_gen).

<p align="center">
    <img src="/doc/wifi_config_QR.PNG" alt="Wifi Config QR" width="500"/>
    <p align="center">Connect to the Access Point using the QR code</p>
</p>

<p align="center">
    <img src="/doc/wifi_config_web.png" alt="Wifi Config QR" width="500"/>
    <p align="center">Web interface to provide wifi credentials</p>
</p>


#### BKK API configuration
After the network is configured, the next step is to provide your BKK API key (can be requested from the official webside of [BKK](https://opendata.bkk.hu/keys)) and the station list to be displayed. 

<p align="center">
    <img src="/doc/api_config_web.png" alt="API config WEB" width="500"/>
    <p align="center">Configure the API in your browser.</p>
</p>

#### Normal operation 
<p align="center">
    <img src="/doc/hw_setup.png" alt="HW_setup" width="400"/>
    <p align="center">HW setup during normal operation</p>
</p>

During normal operation the Display will show the arrivals for the selected stations. Also will indicate if any issue occured. 

## Screenshots 
![Bus Lines presented on the map above](doc/display_demo_01.png)
*Bus Lines presented on the map above.*

![Other stations](doc/display_demo_02.png)

![Other stations](doc/display_demo_03.png)
