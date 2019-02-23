# PDS16 Simulator

This project came as a need to simulate the PDS16 ISA, but also the Interruption routines and the Input/Output ports.

## Features

  - All the PDS16 ISA
  - IN/OUT Ports (Only the SDP16 Ones)
  - Interruption routine simulation
  - Memory Dumps
  - "GDB-like" Interface
  - ASM Reassembler
  - Breakpoints
  - GTK+ GUI (To come)

## Requirements for compilation

This program makes use of the following libraries:

| Library name | GCC Flag | Use |
| ------ | ------ | ------ |
| New Curses (ncurses) | -lncurses | Get the width and height of the console |
| OpenSSH | -lssl -lcrypto | Get the SHA1 of the filename for a future feature |
| Threads | -lpthread | Multithreading for the "auto" command |

The program also depends on the GTK+3 package to run the GTK GUI (once it's finished),
be sure to have all the correct packages in order to compile it with success.

### Installation of the requirements

For Debian-based distributions:

```sh
$ sudo apt-get install gcc make build-essential libncurses-dev libssl-dev libgtk-3-dev
```
For other destributions:

```sh
# Nothing yet... Sorry :(
```

## Downloading and compiling

To download the source-code, be sure to have the "git" package installed and do:

```sh
$ git clone https://www.github.com/fabiossilva21/PDS16
```

This application makes use of the Make package available in, hopefully, every Linux distro. Navigate to the project root folder and:

```sh
$ make
```

### Development

If you want to contribute, feel free to drop suggestions on the [Issues](https://github.com/fabiossilva21/PDS16/issues) sub-page here on Github. If you have a fix for an Issue or you find a certain alteration to the code makes the program better, feel free to make a [Pull Request](https://github.com/fabiossilva21/PDS16/pulls).

## License

Licensed under the [MIT License](LICENSE).