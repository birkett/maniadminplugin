0. Set up and install a Virtual Machine: https://www.virtualbox.org/wiki/Downloads

1. Install Debian 5 (debian-5010-i386-CD-1.iso) - Using newer distro's will cause compiled bins to be incompatible with older distro's.

2. Update Debian 5 "sources.list"

"nano /etc/apt/sources.list"

Replace the contents of the file with:

deb http://archive.debian.org/debian/ lenny main non-free contrib

deb http://archive.debian.org/debian-backports lenny-backports main

deb http://archive.debian.org/debian-backports lenny-backports-sloppy main


Save changes and do "apt-get update". Then you need to run "apt-get install build-essential"
You're installing a compiler to compile the compiler that Mani requires in order to be compiled ;)

3. Download and extract "gcc-4.6.3.tar.bz2" into your home directory "/home/your-name"
Link: https://www.dropbox.com/s/kqmnvxxcnf2oayw/gcc-4.6.3.tar.bz2

4.In your terminal: cd gcc-4.6.3

Now copy and paste the below link into your terminal and press enter

./configure -v --prefix=/usr/local/gcc-4.6 --enable-shared --with-system-zlib --enable-nls --without-included-gettext --program-suffix=-4.6 --enable-cxa\_atexit

5. The following must be installed before continuing:

apt-get install zlib1g-dev

apt-get install bison

apt-get install flex

apt-get install texinfo

6. Your terminal must still be inside "gcc-4.6.3" (cd /home/your-name/gcc-4.6.3). Type "ls" to see a list of the files your terminal is looking at to be sure.

7. Run the following commands in your terminal as "ROOT".

make

make install

Depending on your computer's hardware, the two commands above will take a while.