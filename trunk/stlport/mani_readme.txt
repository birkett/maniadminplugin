The STLPort library has been added so the we can remove the libstdc++ library dependency. Hopefully this will remove some possible issues with the linux builds of MAP.

There are a number of blogs about how screwed up libstdc++ can be, especially when various different statically linked versions are running in the same process.

http://www.bailopan.net/blog/?p=3
http://www.bailopan.net/blog/?p=4
http://www.trilithium.com/johan/2005/06/static-libstdc/
http://pages.cs.wisc.edu/~psilord/blog/2.html

The STLPort library can be downloaded from here http://www.stlport.org/

Once extracted you can run the configure

configure --prefix=$HOME/MyDev/stlport/ --with-cxx=g++-4.1 --with-cc=gcc-4.1 -without-debug -without-stldebug --enable-static 
cd build/lib
make install

Job done.

Mani
