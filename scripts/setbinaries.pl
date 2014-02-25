#!/usr/bin/perl

# Copies binaries to Valve engined games and to a base folder
use Getopt::Std;
use File::Copy;
use File::Basename;
use File::Spec::Functions qw(rel2abs);

sub copy_binaries;
sub create_folder;

#Setup vars here
$BASE_FOLDER=dirname(rel2abs($0)) . "/..";
$CORE_BIN="mani_admin_plugin";
$LINUX_BASE=$BASE_FOLDER;
$WINDOWS_BASE=$BASE_FOLDER;
$ROOT_GAME="/srcds_1";
$PLUGIN_BASE="VSP";

#Setup possible folders where games exist
%folder_type = (
"/csgo"                 => "csgo",
"/css"                  => "css",
"/dod"                  => "dods",
"/hl2mp"                  => "hl2dm",
"/tf"                  => "tf2"

);

%option = ();
getopts("rsocg", \%option);

$SMM_EXT="";
if ($option{r})
{
        print "RELEASE MODE\n";
        $RELEASE="TRUE";
}

if ($option{s})
{
    print "MM:S Version\n";
    $SMM="TRUE";
    $SMM_EXT="_mm";
        $PLUGIN_BASE="SourceMM";
}

if ($option{g}) 
{
        print "CSGO SDK\n";
        $CSGO="TRUE"
}

if ($option{c}) 
{
        print "CSS SDK\n";
        $CSS="TRUE"
}

if ($option{d})
{
    print "DODS SDK\n";
    $DODS="TRUE"
}

if ($option{h})
{
    print "HL2DM SDK\n";
    $HL2DM="TRUE"
}

if ($option{t})
{
    print "TF2 SDK\n";
    $TF2="TRUE"
}


if ($^O eq "MSWin32")
{
        print "Windows platform\n";
        $WINDOWS="TRUE";
        $DEV_BASE=$WINDOWS_BASE;
        $FILE_EXT=".dll";
        $SMM_FILE_EXT=".dll";
        $ARCH="";
}
else
{
#Linux platform
        print "Linux platform\n";
        $DEV_BASE=$LINUX_BASE;
        $FILE_EXT=".so";
        $SMM_FILE_EXT=".so";
        $ARCH="_i486";
}

$BIN_FOLDER=$DEV_BASE . "/plugin_output";

foreach $folder_ext ( keys %folder_type ) 
{
        $DO_COPY = "FALSE";
        if ($CSGO)
        {
                if ($folder_type{ $folder_ext } eq "csgo")
                {
                        $DO_COPY = "TRUE";
                }
        }
        
        if ($CSS)
        {
                if ($folder_type{ $folder_ext } eq "css")
                {
                        $DO_COPY = "TRUE";
                }
        }

        if ($DODS)
        {
                if ($folder_type{ $folder_ext } eq "dods")
                {
                        $DO_COPY = "TRUE";
                }
        }

        if ($HL2DM)
        {
                if ($folder_type{ $folder_ext } eq "hl2dm")
                {
                        $DO_COPY = "TRUE";
                }
        }

        if ($TF2)
        {
                if ($folder_type{ $folder_ext } eq "tf2")
                {
                        $DO_COPY = "TRUE";
                }
        }		
        
		if ($DO_COPY eq "TRUE")
        {
                $ENGINE_BASE=$DEV_BASE . $ROOT_GAME . $folder_ext;

                $BIN_FILE=$CORE_BIN . $SMM_EXT . $ARCH . $SMM_FILE_EXT;
                $PDB_FILE=$CORE_BIN . $SMM_EXT . $ARCH . ".pdb";

                print "\nINFO:\nDEV_BASE:  $DEV_BASE\nENGINE_BASE:  $ENGINE_BASE\nBIN_FOLDER:  $BIN_FOLDER\n";
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
                                        copy_binaries($listitem);       
                                }
                        }
                }
        }
}

#Copy ready for a build
if ($RELEASE)
{
		
        if ($CSGO)
        {
                print "Copying $BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$BIN_FILE to $DEV_BASE/public_build/csgo_bin/$BIN_FILE\n";
                copy ("$BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$BIN_FILE",
                "$DEV_BASE/public_build/csgo_bin/$BIN_FILE");
                if ($^O eq "MSWin32")
                {
                        print "Copying $BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$PDB_FILE to $DEV_BASE/public_build/csgo_bin/$PDB_FILE\n";
                        copy ("$BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$PDB_FILE",
                        "$DEV_BASE/public_build/csgo_bin/$PDB_FILE");
                }
        }
		
        if ($CSS)
        {
                print "Copying $BIN_FOLDER/css_bin/$PLUGIN_BASE/$BIN_FILE to $DEV_BASE/public_build/css_bin/$BIN_FILE\n";
                copy ("$BIN_FOLDER/css_bin/$PLUGIN_BASE/$BIN_FILE",
                "$DEV_BASE/public_build/css_bin/$BIN_FILE");
                if ($^O eq "MSWin32")
                {
                        print "Copying $BIN_FOLDER/css_bin/$PLUGIN_BASE/$PDB_FILE to $DEV_BASE/public_build/css_bin/$PDB_FILE\n";
                        copy ("$BIN_FOLDER/css_bin/$PLUGIN_BASE/$PDB_FILE",
                        "$DEV_BASE/public_build/css_bin/$PDB_FILE");
                }
        }

		if ($DODS)
        {
                print "Copying $BIN_FOLDER/dods_bin/$PLUGIN_BASE/$BIN_FILE to $DEV_BASE/public_build/dods_bin/$BIN_FILE\n";
                copy ("$BIN_FOLDER/dods_bin/$PLUGIN_BASE/$BIN_FILE",
                "$DEV_BASE/public_build/dods_bin/$BIN_FILE");
                if ($^O eq "MSWin32")
                {
                        print "Copying $BIN_FOLDER/dods_bin/$PLUGIN_BASE/$PDB_FILE to $DEV_BASE/public_build/dods_bin/$PDB_FILE\n";
                        copy ("$BIN_FOLDER/dods_bin/$PLUGIN_BASE/$PDB_FILE",
                        "$DEV_BASE/public_build/dods_bin/$PDB_FILE");
                }
        }
		
		if ($HL2DM)
        {
                print "Copying $BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$BIN_FILE to $DEV_BASE/public_build/hl2dm_bin/$BIN_FILE\n";
                copy ("$BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$BIN_FILE",
                "$DEV_BASE/public_build/hl2dm_bin/$BIN_FILE");
                if ($^O eq "MSWin32")
                {
                        print "Copying $BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$PDB_FILE to $DEV_BASE/public_build/hl2dm_bin/$PDB_FILE\n";
                        copy ("$BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$PDB_FILE",
                        "$DEV_BASE/public_build/hl2dm_bin/$PDB_FILE");
                }
        }
		
		if ($TF2)
        {
                print "Copying $BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$BIN_FILE to $DEV_BASE/public_build/tf2_bin/$BIN_FILE\n";
                copy ("$BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$BIN_FILE",
                "$DEV_BASE/public_build/tf2_bin/$BIN_FILE");
                if ($^O eq "MSWin32")
                {
                        print "Copying $BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$PDB_FILE to $DEV_BASE/public_build/tf2_bin/$PDB_FILE\n";
                        copy ("$BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$PDB_FILE",
                        "$DEV_BASE/public_build/tf2_bin/$PDB_FILE");
                }
        }		
}

sleep(1);

#### Functions
sub copy_binaries
{
my $mod_dir = $ENGINE_BASE . "/" . $_[0];
my $search_curly = 0;

        print "Setting up binaries for " . $_[0] . "\n";
        create_folder("$mod_dir/addons");
        create_folder("$mod_dir/addons/mani_admin_plugin");
        create_folder("$mod_dir/addons/mani_admin_plugin/bin");
        create_folder("$mod_dir/addons/metamod/");
        create_folder("$mod_dir/addons/metamod/bin");
        create_folder("$mod_dir/cfg");
        create_folder("$mod_dir/cfg/mani_admin_plugin");


		if ($CSGO)
        {
                copy ("$DEV_BASE/sourcemm_bin/server" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/server" . $SMM_FILE_EXT);
                copy ("$DEV_BASE/sourcemm_bin/metamod.2.csgo" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/metamod.2.csgo" . $SMM_FILE_EXT);
        }		

        if ($CSS)
        {
                #Copy Meta Mod Source 1.10.x binary
                copy ("$DEV_BASE/sourcemm_bin/server" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/server" . $SMM_FILE_EXT);
                copy ("$DEV_BASE/sourcemm_bin/metamod.2.css" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/metamod.2.css" . $SMM_FILE_EXT);
        }
		
        if ($DODS)
        {
                #Copy Meta Mod Source 1.10.x binary
                copy ("$DEV_BASE/sourcemm_bin/server" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/server" . $SMM_FILE_EXT);
                copy ("$DEV_BASE/sourcemm_bin/metamod.2.dods" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/metamod.2.dods" . $SMM_FILE_EXT);
        }

        if ($HL2DM)
        {
                #Copy Meta Mod Source 1.10.x binary
                copy ("$DEV_BASE/sourcemm_bin/server" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/server" . $SMM_FILE_EXT);
                copy ("$DEV_BASE/sourcemm_bin/metamod.2.hl2dm" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/metamod.2.hl2dm" . $SMM_FILE_EXT);
        }

        if ($TF2)
        {
                #Copy Meta Mod Source 1.10.x binary
                copy ("$DEV_BASE/sourcemm_bin/server" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/server" . $SMM_FILE_EXT);
                copy ("$DEV_BASE/sourcemm_bin/metamod.2.tf2" . $SMM_FILE_EXT,
                        "$mod_dir/addons/metamod/bin/metamod.2.tf2" . $SMM_FILE_EXT);
        }		


        if ($CSGO)
        {
                if ($SMM)
                {
                        print "Copying $BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE");
                }
                else
                {
                        print "Copying $BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/csgo_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/$BIN_FILE");
                }
        }
		
		if ($CSS)
        {
                if ($SMM)
                {
                        print "Copying $BIN_FOLDER/css_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/css_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE");
                }
                else
                {
                        print "Copying $BIN_FOLDER/css_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/css_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/$BIN_FILE");
                }
        }

		if ($DODS)
        {
                if ($SMM)
                {
                        print "Copying $BIN_FOLDER/dods_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/dods_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE");
                }
                else
                {
                        print "Copying $BIN_FOLDER/dods_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/dods_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/$BIN_FILE");
                }
        }

		if ($HL2DM)
        {
                if ($SMM)
                {
                        print "Copying $BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE");
                }
                else
                {
                        print "Copying $BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/hl2dm_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/$BIN_FILE");
                }
        }

		if ($TF2)
        {
                if ($SMM)
                {
                        print "Copying $BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/mani_admin_plugin/bin/$BIN_FILE");
                }
                else
                {
                        print "Copying $BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$BIN_FILE to $mod_dir/addons/$BIN_FILE\n";
                        copy ("$BIN_FOLDER/tf2_bin/$PLUGIN_BASE/$BIN_FILE",
                                "$mod_dir/addons/$BIN_FILE");
                }
        }		

        #Parse gameinfo.txt and setup for correct mode
        open(DAT, "$mod_dir/gameinfo.txt");
        @raw_data=<DAT>;
        close(DAT);

        if ($SMM)
        {       
                unlink("$mod_dir/addons/mani_admin_plugin.vdf");

                open(DAT, ">$mod_dir/addons/metamod.vdf");
                print DAT "\"Plugin\"\n";
                print DAT "{\n";
                print DAT "\t\"file\"\t\"../$_[0]/addons/metamod/bin/server\"\n";
                print DAT "}\n";
                close(DAT);

                open(DAT, ">$mod_dir/addons/metamod/$CORE_BIN.vdf");
                print DAT "\"Metamod Plugin\"\n";
                print DAT "{\n";
                print DAT "\t\"file\"\t\"addons/mani_admin_plugin/bin/$CORE_BIN$SMM_EXT\"\n";
                print DAT "}\n";
                close(DAT);
        }
        else
        {               
                unlink("$mod_dir/addons/metamod.vdf");

                open(DAT, ">$mod_dir/addons/$CORE_BIN.vdf");
                print DAT "\"Plugin\"\n";
                print DAT "{\n";
                print DAT "\t\"file\" \"../$_[0]/addons/".$CORE_BIN.$ARCH."\"\n";

                print DAT "}\n";
                close(DAT);
        }

}

sub create_folder
{
        if (not -d $mod_dir . $_[0])
        {
                print "Adding folder $mod_dir" . $_[0] . "\n";
                mkdir $mod_dir . $_[0];
        }
}