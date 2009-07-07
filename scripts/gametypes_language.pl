#!/usr/bin/perl

# Copies gametypes and english.cfg to Valve engined games and to a base folder

use File::Copy;

sub do_copy;
sub copy_defaults;
sub create_folder;

#Setup vars here

$LINUX_BASE=$ENV{HOME}."/MyDev";
$WINDOWS_BASE="C:/MyDev";
$ROOT_GAME="/srcds_1";
$ROOT_ORANGE_GAME="/srcds_1/orangebox";

if ($^O eq "MSWin32")
{
	print "Windows platform\n";
	$WINDOWS="TRUE";
	$DEV_BASE=$WINDOWS_BASE;
}
else
{
#Linux platform
	print "Linux platform\n";
	$DEV_BASE=$LINUX_BASE;
}


$ENGINE_BASE=$DEV_BASE . $ROOT_ORANGE_GAME;
do_copy();

$ENGINE_BASE=$DEV_BASE . $ROOT_GAME;
do_copy();

	
sleep(3);

sub do_copy
{
opendir MYDIR, "$ENGINE_BASE/";
@contents = grep !/^\.\.?$/, readdir MYDIR;
closedir MYDIR;

foreach $listitem ( @contents )
{
	if ( -d $ENGINE_BASE . "/" . $listitem && $listitem ne "hl2")
	{
		if ( -e $ENGINE_BASE . "/" . $listitem . "/gameinfo.txt")
		{
			copy_defaults($listitem);	
		}
	}
}
}


#### Functions
sub copy_defaults
{
my $mod_dir = $ENGINE_BASE . "/" . $_[0];


	print "Setting up gametypes.txt and language.txt for " . $_[0] . "\n";
	create_folder("$mod_dir/cfg");
	create_folder("$mod_dir/cfg/mani_admin_plugin");
	create_folder("$mod_dir/cfg/mani_admin_plugin/language");

	copy ("$DEV_BASE/public_build/config_files/cfg/mani_admin_plugin/gametypes.txt",
		"$mod_dir/cfg/mani_admin_plugin/gametypes.txt");

	copy ("$DEV_BASE/public_build/config_files/cfg/mani_admin_plugin/language/english.cfg",
		"$mod_dir/cfg/mani_admin_plugin/language/english.cfg");


}

sub create_folder
{
	if (not -d $mod_dir . $_[0])
	{
		print "Adding folder $mod_dir" . $_[0] . "\n";
		mkdir $mod_dir . $_[0];
	}
}
