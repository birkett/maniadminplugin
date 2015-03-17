1.   Install VirtualBox from http://www.virtualbox.org/

2.   Get the 9.04 32bit Desktop version of Ubuntu from http://www.ubuntu.com/getubuntu/download

3.   Install Ubuntu using VirtualBox (make sure the network settings on VirtualBox is set to bridged) you should probably give yourself 512MB memory and a 25Gig harddrive too on the settings.

4.   Once Ubuntu is installed, start up Ubuntu and in the network settings, set your ip address/mask/gateway and primary dns (It is useful to keep a static IP).

5.   Open up the synaptic package manager

7.   Install the following packages: -

  * ~~gcc-4.1~~ gcc-3.4.6
  * ~~g++4.1~~ g++-3.4.6
  * openssh
  * tofrodos
  * subversion
  * valgrind
  * alleyoop
  * kcachegrind
  * kdesvn

8.   Install putty on your windows pc http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html

9.   Use putty to ssh into your server.

10.   In .profile in $HOME add the following two lines to the bottom of the file

  * PATH="./:$PATH"
  * PATH="$HOME/MyDev/scripts/:$PATH"

11.   **mkdir MyDev**

12.   Use kdesvn in the linux GUI to checkout the code from the google repository (you can right click on the MyDev folder and select ‘Open with kdesvn’).

13.   In the putty client , cd $HOME/MyDev/scripts/

14.   Set the scripts to have execute permissions by using **chmod +x**

15.   Now we need to get the libraries for the SDKs

16.   cd ../sdk/

17.   **wget http://www.mani-admin-plugin.com/mani_dev/sdk_libs/sdk_lib.zip***

18.   **unzip sdk\_lib.zip**

19.   **cd ../sdk\_orange/**

20.   **wget http://www.mani-admin-plugin.com/mani_dev/sdk_libs/sdk_orange_lib.zip***

21.   **unzip sdk\_orange\_lib.zip**

22.   Run the following script to install the game servers, **updatesrcds.pl**. Repeat this until you are up to date with the latest versions. Note that you will need to install the MAP config files as per a normal server.

23.   To build the plugin type **bp.sh –c** then select the plugin version to build from the menu (Option **–c** is for a clean build, option **–d** builds with debug information in the plugin)

24.   To start a server run **start.pl** and enter the server type to run.

25.   To change from VSP to SourceMM run **settype.pl** for VSP mode and **settype.pl –s** for SMM mode.