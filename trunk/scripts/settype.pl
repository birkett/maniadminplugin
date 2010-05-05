#!/usr/bin/perl

# Copies binaries to Valve engined games and to a base folder
use Getopt::Std;
use File::Copy;
use File::Basename;
use File::Spec::Functions qw(rel2abs);

sub copy_binaries;
sub create_folder;

#Setup vars here
$BASE_FOLDER=dirname(rel2abs($0)) . "/../";
$CORE_BIN="mani_admin_plugin";
$LINUX_BASE=$BASE_FOLDER;
$WINDOWS_BASE=$BASE_FOLDER;
$ROOT_GAME="/srcds_1";
$ROOT_ORANGE_GAME="/srcds_1/orangebox";

%option = ();
getopts("s", \%option);

$SMM_EXT="";
if ($option{s})
{
    print "MM:S Version\n";
    $SMM="TRUE";
    $SMM_EXT="_mm";
}

if ($^O eq "MSWin32")
{
	print "Windows platform\n";
	$WINDOWS="TRUE";
	$DEV_BASE=$WINDOWS_BASE;
	$FILE_EXT=".dll";
	$BIN_FOLDER=$DEV_BASE . "/plugin_output";
}
else
{
#Linux platform
	print "Linux platform\n";
	$DEV_BASE=$LINUX_BASE;
	$BIN_FOLDER=$DEV_BASE . "/mani_admin_plugin";#
	$FILE_EXT="_i486.so";
}


$ENGINE_BASE=$DEV_BASE . $ROOT_ORANGE_GAME;
$BIN_FILE=$CORE_BIN . $SMM_EXT . $FILE_EXT;

print "$DEV_BASE\n$ENGINE_BASE\n$BIN_FOLDER\n";
print "File = " . $BIN_FILE . "\n";

opendir MYDIR, "$ENGINE_BASE/";
@contents = grep !/^\.\.?$/, readdir MYDIR;
closedir MYDIR;

foreach $listitem ( @contents )
{
	if ( -d $ENGINE_BASE . "/" . $listitem && $listitem ne "hl2")
	{
		if ( -e $ENGINE_BASE . "/" . $listitem . "/gameinfo.txt")
		{
			copy_binaries($listitem, "TRUE");	
		}
	}
}


$ENGINE_BASE=$DEV_BASE . $ROOT_GAME;
$BIN_FILE=$CORE_BIN . $SMM_EXT . $FILE_EXT;

print "$DEV_BASE\n$ENGINE_BASE\n$BIN_FOLDER\n";
print "File = " . $BIN_FILE . "\n";

opendir MYDIR, "$ENGINE_BASE/";
@contents = grep !/^\.\.?$/, readdir MYDIR;
closedir MYDIR;

foreach $listitem ( @contents )
{
	if ( -d $ENGINE_BASE . "/" . $listitem && $listitem ne "hl2")
	{
		if ( -e $ENGINE_BASE . "/" . $listitem . "/gameinfo.txt")
		{
			copy_binaries($listitem, "FALSE");	
		}
	}
}
	
	
sleep(1);

#### Functions
sub copy_binaries
{
my $mod_dir = $ENGINE_BASE . "/" . $_[0];
my $search_curly = 0;
my $orange_copy = $_[1];


	print "Setting up mod " . $_[0] . "\n";
	
	#Parse gameinfo.txt and setup for correct mode
	open(DAT, "$mod_dir/gameinfo.txt");
	@raw_data=<DAT>;
	close(DAT);

	if ($SMM)
	{	
		if (not grep(/metamod/, @raw_data))
		{
			open(DAT, ">$mod_dir/gameinfo.txt");

			$next_line = 0;
			foreach $LINE_VAR (@raw_data)
			{
				if (grep(/SearchPaths/, $LINE_VAR))
				{
					$search_curly = 1;
				}
				else
				{
					if ($search_curly ne 0)
					{
						if (grep(/{/, $LINE_VAR))
						{
							$search_curly = 1;
						}				
						else
						{
							$search_curly = 2;
						}
					}
				}
				

				if ($search_curly eq 2)
				{
					print DAT "                              GameBin                         |gameinfo_path|addons/metamod/bin\n";
					$search_curly = 0;
				}

				print DAT $LINE_VAR;
			}
			close(DAT);
		}

		unlink("$mod_dir/addons/mani_admin_plugin.vdf");
	}
	else
	{		
		if (grep(/metamod/, @raw_data))
		{
			open(DAT, ">$mod_dir/gameinfo.txt");
			foreach $LINE_VAR (@raw_data)
			{
				if (not grep(/metamod/, $LINE_VAR))
				{
					print DAT $LINE_VAR;
				}
			}
			close(DAT);
		}

		open(DAT, ">$mod_dir/addons/$CORE_BIN.vdf");
		print DAT "\"Plugin\"\n";
		print DAT "{\n";
		if ($orange_copy eq "TRUE")
		{
			print DAT "\t\"file\" \"../$_[0]/addons/" . $CORE_BIN . "_i486\"\n";
		}
		else
		{
			print DAT "\t\"file\" \"../$_[0]/addons/$CORE_BIN\"\n";
		}

		print DAT "}\n";
		close(DAT);
	}

}

