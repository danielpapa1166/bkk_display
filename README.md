<p align="center">
    <img src="doc/header_gif.gif" alt="Header Gif" width="500"/>
    <p align="center">Routes from "Diószegi út (Vérellátó szolgálat)" and "Hollókő utca" to the city center.</p>
</p>

# BKK Display

BKK Display is a Yocto-based project that uses a custom BKK API submodule to fetch real-time public transport data from the Budapest Public Transport Center’s key-accessed server. With a Qt-powered UI, it lets you display and configure arrival times for selected stations, offering a simple and personalized solution for commuting.

## Motivation

Living in Budapest, I have two bus stations near my home, each in opposite directions from my apartment. These stations provide me with options for getting into the city for work, socializing with friends, or running daily tasks. However, I often need a quick way to decide which station to use, depending on the buses’ real-time schedules. This little project was born out of this daily question, and it’s made life just a bit smoother.

![Bus Stations Map](doc/routes_to_the_city.png)  
*A map illustrating the two lines I frequently use.*

## Architecture 
Description of modules used in this project 

## Getting started

### BOM 
- Raspberry Pi 4 
- Display module: todo: link

### SW setup 
- build and flash chain and tools 

### User config 
- wifi config
- BKK api config 
- normal operation 

<p align="center">
    <img src="/doc/hw_setup.png" alt="HW_setup" width="400"/>
    <p align="center">HW setup during normal operation</p>
</p>

## Screenshots 
![Bus Lines presented on the map above](doc/display_demo_01.png)
*Bus Lines presented on the map above.*

![Other stations](doc/display_demo_02.png)

![Other stations](doc/display_demo_03.png)
