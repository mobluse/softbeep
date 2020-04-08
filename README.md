# softbeep 0.3 - Software Bell for Linux

Original README below. I didn't orginally develop this system. I only fixed so that it could compile in Raspbian Buster Linux (based on Debian)
and improved the shell scripts using inspiration from others. I put it here, because it was not already on GitHub.

https://raspberrypi.stackexchange.com/questions/107403/i-dont-get-a-system-bell-in-lxterminal-on-raspberry-pi-4b-raspbian  
https://raspberrypi.stackexchange.com/questions/8927/enabling-audible-terminal-bell-beep-on-wheezy  
http://0pointer.de/lennart/projects/softbeep/  
https://github.com/aur-archive/softbeep  

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
softbeep 0.3 - Software Bell for Linux
======================================

Copyright 2002
Lennart Poettering <mz736f667462656570@poettering.de>

---------------------------------------------------------------------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

---------------------------------------------------------------------

This Linux utility program may be used for redirecting beeps of the
internal PC speaker to your sound card. It is implemented entirely in
user space by using LD_PRELOAD. It catches four different types of
beeps, which can be produced on a Linux system:

1) BEL-characters (#7) written to your TTYs, which should normaly
   result in a beep executed by your terminal emulator

2) Calls to gdk_bell() by GTK/Gnome based programs

3) Calls to XBell() by Xlib based programs

4) Calls to beep() by curses based programs

These together catch most of the beeps of most of the programs I use.

The reason for developing this tool is that the internal speaker of
one of my computers is broken on some way. Unfortunately life is far
less fun without those nifty beeps and bells played by your PC.

Usage
-----

You need to tell the dynamic loader of Linux to preload the library
libsoftbeep.so for all processes. This is done by setting the
LD_PRELOAD environment variable to the path of the library.

You may use the shell script "softbeep" shipped with the distribution
for accomplishing this task. Just run "softbeep foo" for running
program foo with redirected beeps. Call softbeep without parameters
for getting a shell from which every program run will get redirected
beeps.

Two environment variables SB_REMOVE_BEL and SB_RUN may be used for
adjusting softbeep to your needs. When SB_REMOVE_BEL is set to "yes"
every catched BEL character written to a TTY is dropped, otherwise it
is passed to the next layer. SB_RUN specifies the program to run when
a beep occurs. You may adjust these parameters to your needs in the
top of the script softbeep. "sb-beep" (a short script playing a
wavefile via esdplay, which is shipped with the EsounD distribution)
is used for emitting a beep by default. You might want to adjust this
script to your individual needs, e.g. for playing different wave files
for different programs.

Notes
-----

softbeep has not been tested intensively with multi threaded
programs yet; may be it works seamlessly, maybe it does not.

Processes with access to several different TTYs at the same time may
not be handled correctly. This is a minor bug, since there are only
very few programs which make use of more than one TTY at once.

SUID/SGID programs like xterm do not work with softbeep. This is a
limitation of LD_PRELOAD (due to security considerations) and not a
bug in softbeep! Solution for xterm: For catching all the beeps of
xterm you should preload the library to the shell running inside of
the xterm, which is not SUID/SGID.

ssh is a SUIG/SGID program on several installations. I currently do not
know a way how to work around this.

Development
-----------

Development was done under Debian GNU Linux Sid for i386 from March
2002.

Compatibility
-------------

This program is compatible with GNU libc 2.2.5 under Linux. It should
not be too difficult to port it to other systems, but it is
incompatible as is.

Installation
------------

Run "make" for compiling the program. You might want to install it
permanently on your system by issuing "make install" as root. This
installs softbeep to /usr/local/. (Edit the Makefile for another
prefix) For removing this installation you might want to try "make
deinstall" as root.

The compilation needs installed X11 headers, alltough the library will
not be linked against Xlib. Something similar is true for
curses. Gtk/Gdk headers are not needed.

Internet
--------

You may find up to date releases of this utility on
	http://www.stud.uni-hamburg.de/users/lennart/projects/softbeep/

You may download this release from
	http://www.stud.uni-hamburg.de/users/lennart/projects/softbeep/softbeep-0.3.tar.gz

Thanks go to
------------

Manish Singh <yosh@gimp.org> for writing esddsp on which my code is
based.

---------------------------------------------------------------------

Lennart Poettering <mz736f667462656570@poettering.de>, 2002
