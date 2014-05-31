#!/usr/bin/perl

# Creates a build release of CSGO, CSS, DOD:S, HL2:DM or TF2
use File::Copy;
use File::Copy::Recursive qw(fcopy rcopy dircopy fmove rmove dirmove);
use Archive::Zip;   # imports
use File::Basename;
use File::Spec::Functions qw(rel2abs);

sub cleanup;
sub recursedir($$);

#Setup Vars here
$BASE_FOLDER=dirname(rel2abs($0)) . "/..";
$RELEASE_BASE=$BASE_FOLDER . "/public_build";

print "Is this an CS:(G)O build, (C)SS build, (D)OD:S build, (H)L2:DM build or (T)F2 build? ";
$question_response = <>;
chomp($question_response);

if ($question_response eq "g" || $question_response eq "G")
{
	print "CSGO Mode\n";
	$CSGO="TRUE";
	$BINARY_LOC="$RELEASE_BASE/csgo_bin";
	$EXTRA_FILES="$RELEASE_BASE/game_specific/csgo";
}
elsif ($question_response eq "c" || $question_response eq "C")
{
	print "CSS Mode\n";
	$CSS="TRUE";
	$BINARY_LOC="$RELEASE_BASE/css_bin";
	$EXTRA_FILES="$RELEASE_BASE/game_specific/css";
}
elsif ($question_response eq "d" || $question_response eq "D")
{
	print "DODS Mode\n";
	$DODS="TRUE";
	$BINARY_LOC="$RELEASE_BASE/dods_bin";
	$EXTRA_FILES="$RELEASE_BASE/game_specific/dods";
}
elsif ($question_response eq "h" || $question_response eq "H")
{
	print "HL2DM Mode\n";
	$DODS="TRUE";
	$BINARY_LOC="$RELEASE_BASE/hl2dm_bin";
	$EXTRA_FILES="$RELEASE_BASE/game_specific/hl2dm";
}
elsif ($question_response eq "t" || $question_response eq "T")
{
	print "TF2 Mode\n";
	$DODS="TRUE";
	$BINARY_LOC="$RELEASE_BASE/tf2_bin";
	$EXTRA_FILES="$RELEASE_BASE/game_specific/tf2";
}
else
{
	print "ERROR!\n";
	exit;
}

print "What is the version, i.e v1_2_22_15? ";
$question_response = <>;
chomp($question_response);

if ($CSGO)
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_csgo.zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_csgo_pdb.zip";
}
elsif ($CSS)
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_css.zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_css_pdb.zip";
}
elsif ($DODS)
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_dods.zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_dods_pdb.zip";
}
elsif ($HL2DM)
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_hl2dm.zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_hl2dm_pdb.zip";
}
elsif ($TF2)
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_tf2.zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_tf2_pdb.zip";
}

# Remove any temporary files
$TMP_FOLDER=$RELEASE_BASE ."/tmp";

cleanup($TMP_FOLDER);
mkdir $TMP_FOLDER;

$SOURCE_COPY=$RELEASE_BASE. "/config_files";
$TARGET_COPY=$RELEASE_BASE. "/tmp";

print "$SOURCE_COPY\n";
print "$TARGET_COPY\n";

$numoffiles = dircopy($SOURCE_COPY,$TARGET_COPY) or die $!;
print "Copied $numoffiles files into /tmp\n";

$numoffiles = 0;
$numoffiles = dircopy($EXTRA_FILES,$TARGET_COPY) or die $!;
print "Copied an additional $numoffiles game specific files into /tmp\n";

print "Copying binaries from $BINARY_LOC\n";

copy ("$BINARY_LOC/mani_admin_plugin_mm.dll",
	"$RELEASE_BASE/tmp/addons/mani_admin_plugin/bin/mani_admin_plugin_mm.dll");
	
copy ("$BINARY_LOC/mani_admin_plugin_mm_i486.so",
	"$RELEASE_BASE/tmp/addons/mani_admin_plugin/bin/mani_admin_plugin_mm_i486.so");

copy ("$BINARY_LOC/mani_admin_plugin.dll",
	"$RELEASE_BASE/tmp/addons/mani_admin_plugin.dll");
	
copy ("$BINARY_LOC/mani_admin_plugin_i486.so",
	"$RELEASE_BASE/tmp/addons/mani_admin_plugin_i486.so");

chdir ($TARGET_COPY);

my($dir)      = '.';   #  start in the current dir
my($filelist) = [];   #  a blank array ref

recursedir $dir, $filelist;

#  then print it
#

print $_, "\n" for(@$filelist);

$obj = Archive::Zip->new();   # new instance

foreach $file (@$filelist) {
    print "$file\n";
    
    $obj->addFile($file);   # add files
}

unlink($FINAL_FILE);

if ($obj->writeToFileNamed($FINAL_FILE) != AZ_OK) {  # write to disk
    print "Error in archive $FINAL_FILE creation!\n";
} else {
    print "Archive $FINAL_FILE created successfully!\n";
} 


# Make up the same but with the pdb files included too
if ($^O eq "MSWin32")
{
	copy ("$BINARY_LOC/mani_admin_plugin.pdb",
		"$RELEASE_BASE/tmp/mani_admin_plugin.pdb");	
	copy ("$BINARY_LOC/mani_admin_plugin_mm.pdb",
		"$RELEASE_BASE/tmp/mani_admin_plugin_mm.pdb");	
}

($dir)      = '.';   #  start in the current dir
($filelist) = [];   #  a blank array ref

recursedir $dir, $filelist;

#  then print it
#

print $_, "\n" for(@$filelist);

$obj = Archive::Zip->new();   # new instance

foreach $file (@$filelist) {
    print "$file\n";
    
    $obj->addFile($file);   # add files
}

unlink($FINAL_FILE_PDB);

if ($obj->writeToFileNamed($FINAL_FILE_PDB) != AZ_OK) {  # write to disk
    print "Error in archive $FINAL_FILE_PDB creation!\n";
} else {
    print "Archive $FINAL_FILE_PDB created successfully!\n";
} 




sleep(3);
exit 0;


sub recursedir($$)
{
  my($dir)  = shift @_;
  my($list) = shift @_;

  if(opendir(DIR, "$dir")) {
    #  get files, skipping hidden . and ..
    #
    for my $file(grep { !/^\./ } readdir DIR) {
      if(-d "$dir/$file") {
        #  recurse subdirs
        #
        recursedir "$dir/$file", $list;
      }
      elsif(-f "$dir/$file") {
        #  add files
        #
        push @$list, "$dir/$file";
      }
    }
    closedir DIR;
  }
  else {
    warn "Cannot open dir '$dir': $!\n";
  }
}




sub cleanup {
        my $dir = shift;
	local *DIR;

	opendir DIR, $dir or die "opendir $dir: $!";
	my $found = 0;
	while ($_ = readdir DIR) {
	        next if /^\.{1,2}$/;
	        my $path = "$dir/$_";
		unlink $path if -f $path;
		cleanup($path) if -d $path;
	}
	closedir DIR;
	rmdir $dir or print "error - $!\n";
}

