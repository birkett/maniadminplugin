#!/usr/bin/perl

# Creates a build release of either Orange or Legacy
use File::Copy;
use File::Copy::Recursive qw(fcopy rcopy dircopy fmove rmove dirmove);
use Archive::Zip;   # imports

sub cleanup;
sub recursedir($$);

#Setup Vars here
$RELEASE_BASE="C:/MyDev/public_build";

print "Is this an Orange build? ";
$question_response = <>;
chomp($question_response);

if ($question_response eq "y" || $question_response eq "Y")
{
	print "Orange Mode\n";
	$ORANGE="TRUE";
	$BINARY_LOC="$RELEASE_BASE/orange_bin";
}
else
{
	print "Legacy Mode\n";
	$BINARY_LOC="$RELEASE_BASE/legacy_bin";
}	

print "What is the version, i.e v1_2_beta_s ? ";
$question_response = <>;
chomp($question_response);

if ($ORANGE)
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_orange.zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_orange_pdb.zip";
}
else
{
	$FINAL_FILE=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . ".zip";
	$FINAL_FILE_PDB=$RELEASE_BASE . "/release_zips/mani_admin_plugin_" . $question_response . "_pdb.zip";
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
print "Copied $numoffiles into /tmp\n";

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
    print "Error in archive $FINAL_FILE creation!";
} else {
    print "Archive $FINAL_FILE created successfully!";
} 


# Make up the same but with the pdb files included too
if ($ORANGE eq "TRUE")
{
	copy ("$RELEASE_BASE/../plugin_output/Release/mani_admin_plugin.pdb",
		"$RELEASE_BASE/tmp/mani_admin_plugin.pdb");	
	copy ("$RELEASE_BASE/../plugin_output/ReleaseSMM/mani_admin_plugin_mm.pdb",
		"$RELEASE_BASE/tmp/mani_admin_plugin_mm.pdb");	
}
else
{
	copy ("$RELEASE_BASE/../plugin_output/orange/ReleaseVSP/mani_admin_plugin.pdb",
		"$RELEASE_BASE/tmp/mani_admin_plugin.pdb");	
	copy ("$RELEASE_BASE/../plugin_output/oramge/ReleaseSMM/mani_admin_plugin_mm.pdb",
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
    print "Error in archive $FINAL_FILE_PDB creation!";
} else {
    print "Archive $FINAL_FILE_PDB created successfully!";
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
	rmdir $dir or print "error - $!";
}

