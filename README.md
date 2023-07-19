# Crescendo
![Logo](icon.svg "Crescendo logo")  
Crescendo is a music player controller that can retrieve information about a player via dbus in real-time, control it, and change the output sound device for this player using PulseAudio or PipeWire.

## Features
* Real-time information retrieval for any player via dbus
* Control the player via dbus commands
* Change the output sound device for the player via PulseAudio or PipeWire
* Intuitive graphical user interface

## Dependencies:
* C++17
* gtkmm-4.0
* pugixml 
* PulseAudio or PipeWire(optional, you will not be able to change the output sound device for player)
* [sdbus-c++](https://github.com/Kistler-Group/sdbus-cpp) (optional, without it you will not be able to control another players)
* SDL2, SDL2_mixer, taglib (optional, you will not be able to use Crescendo as local player for audio files)
* [Rohrkabel](https://github.com/Soundux/rohrkabel) (optional, if PipeWire found fill be fetched automatically)

## Installation
To install Crescendo, follow these steps:
1. Clone the project from the GitHub repository:  
```bash
$ git clone https://github.com/PolisanTheEasyNick/Crescendo.git
```
2. Install the dependencies required for the project  
You need to do it specifially for your package manager.  
For example, for Arch Linux:
```bash
$ sudo pacman -Sy pugixml gtkmm-4.0 dbus pulseaudio sdl2 sdl2_mixer taglib 
```
Don't forget that PulseAudio, SDL2, SDL2_mixer, taglib packages are optional.  
Also, you need to install sdbus-c++, which is not available in Arch Linux package repository, so you need to refer to https://github.com/Kistler-Group/sdbus-cpp#building-and-installing-the-library  
But sdbus-c++ is available in Arch User Repository (AUR). You can install sdbus-c++ using your favourite AUR helper or manually:
```bash
$ git clone https://aur.archlinux.org/sdbus-cpp.git
$ cd sdbus-cpp
$ makepkg -si
```
3. Build the project:
```bash
$ cd Crescendo
$ mkdir build && cd build
$ cmake ..
$ make
```
4. Run the application:
```bash
$ ./Crescendo
```

## Usage
When you run Crescendo, you will see a graphical user interface that looks like default player. You can select a player by clicking the `Player` button. The information from choosed player will be displayed in real-time in the Crescendo window.  

You can control the player by using the controls provided in the Crescendo window. You can also change the output sound device for the player by clicking the button with headphones icon and selecting the desired output device from the dropdown menu.

## Contributing
To contribute to Crescendo, follow these steps:

1. Fork the repository from GitHub.
2. Clone the forked repository to your local machine.
3. Create a new branch for your changes.
4. Make your changes and commit them.
5. Push your changes to your forked repository.
6. Submit a pull request to the original repository.

If you encounter any issues or have ideas for new features, please **[open an issue](https://github.com/PolisanTheEasyNick/Crescendo/issues)** on GitHub.




