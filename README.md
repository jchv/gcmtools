README - or everything you wanted to know about this source package but were
afraid to ask

How to Get Source from Git
==========================
### Windows
*   install git from http://git-scm.com/
    * recommended: allow git to modify `%PATH%` for git command ONLY (2nd option)
*   open cmd.exe

```
cd C:\wherever\you\want\to\put\the\source\code\at
git clone [repo URL]
```

### Debian/Ubuntu Linux
```
sudo apt-get install git
cd ~/
git clone [repo URL]
```

How To Build
============
Setup environment
-----------------
### Windows
*   grab 7z file from here:

    http://sourceforge.net/projects/mingwbuilds/files/host-windows/releases/4.7.2/

    recommended configuration: 32-bit / threads-win32 / sljl

*   extract into C:\, such that you have a mingw folder with a bin folder

### Debian/Ubuntu Linux
```
$ sudo apt-get install build-essential
```

Build
-----
### Windows
*   open cmd.exe

```
set PATH=%PATH%;C:\mingw\bin
cd C:\wherever\you\have\the\source\code\at
mingw-make
```

### Linux
```
make
```

Run!


ABOUT GCM.HPP
=============
This is just a quick gcm/fst manipulation library, it is all ISC licensed.
