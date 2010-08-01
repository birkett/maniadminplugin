#!/usr/bin/perl

use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

chdir('..');
chdir('..');

our $SSH = 'ssh -i ../../mmspvkey';

open(PDBLOG, '../OUTPUT/pdblog.txt') or die "Could not open pdblog.txt: $!\n";

#Sync us up with the main symbol store
rsync('sourcemm@alliedmods.net:~/public_html/symbols/', '..\\..\\symstore');

#Get version info
my ($version);
$version = Build::ProductVersion(Build::PathFormat('product.version'));
$version =~ s/-dev//g;
$version .= '-hg' . Build::HgRevNum('.');

my ($build_type);
$build_type = Build::GetBuildType(Build::PathFormat('support/buildbot/build_type'));

if ($build_type eq "dev")
{
	$build_type = "buildbot";
}
elsif ($build_type eq "rel")
{
	$build_type = "release";
}

my ($line);
while (<PDBLOG>)
{
	$line = $_;
	$line =~ s/\.pdb/\*/;
	chomp $line;
	Build::Command("symstore add /r /f \"..\\OUTPUT\\$line\" /s ..\\..\\symstore /t \"Metamod:Source\" /v \"$version\" /c \"$build_type\"");
}

close(PDBLOG);

#Lowercase DLLs.  Sigh.
my (@files);
opendir(DIR, "..\\..\\symstore");
@files = readdir(DIR);
closedir(DIR);

my ($i, $j, $file, @subdirs);
for ($i = 0; $i <= $#files; $i++)
{
	$file = $files[$i];
	next unless ($file =~ /\.dll$/);
	next unless (-d "..\\..\\symstore\\$file");
	opendir(DIR, "..\\..\\symstore\\$file");
	@subdirs = readdir(DIR);
	closedir(DIR);
	for ($j = 0; $j <= $#subdirs; $j++)
	{
		next unless ($subdirs[$j] =~ /[A-Z]/);
		Build::Command("rename ..\\..\\symstore\\$file\\" . $subdirs[$j] . " " . lc($subdirs[$j]));
	}	
}

#Now that we're done, rsync back.
rsync('../../symstore/', 'sourcemm@alliedmods.net:~/public_html/symbols');

sub rsync
{
	my ($from, $to) = (@_);
	
	Build::Command('rsync -av --delete -e="' . $SSH . '" ' . $from . ' ' . $to);
}
