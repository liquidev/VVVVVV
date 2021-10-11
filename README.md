# VVVVVV - PSP port

This is a port of VVVVVV to PlayStation Portable. Still highly work in progress!

## But hey, it runs!

I wouldn't say it runs particularly well, but if you want to enjoy VVVVVV on your PSP in _\~glorious\~_ 15 frames per second (or lower), here's how to build the thing.

```sh
$ ./build.sh -j12  # adjust job count accordingly to suit your hardware
```

That's it!

…

Well, you do actually need the PSP SDK to build this, so don't forget to [install that](https://github.com/pspdev/pspsdk). After that all should be smooth sailin' :ok_hand:

After you build the game, plug in your PSP in USB mode, and navigate to `/PSP/GAME`. There, create a folder `vvvvvv` and put `build/EBOOT.PBP` in it. Also, grab the data.zip from your legit copy of the game and put it there, too.

Note that in the future this process may become a little bit more complicated, as soon enough I'll need to do some repacking of the data.zip to lessen the amount of work that has to be done at startup, thus reducing loading times.
