#!/bin/perl

use Net::FTP;
sub transfer_file;

$bsize=1024 * 100;


$host_name="";

#Core dev paths
$linux_dev_base='$HOME/MyDev';
$windows_dev_base="C:/MyDev";

#Path on remote ftp server
$upload_target="";

$no_of_args = $#ARGV + 1;

$user_login=$ARGV[0];
$user_pass=$ARGV[1];

if ($no_of_args eq 2)
{
	print "Name the build to upload (i.e v1_2_beta_s) ";
	$build_wildcard = <STDIN>;
	chomp($build_wildcard);
}
elsif ($no_of_args eq 3)
{
	$build_wildcard = $ARGV[2];
}
else
{
	print "Usage : $0 user_name user_pass (optional build wildcard)\n";
	exit 1;
}

# Http path that will be used
$http_url="http://www.mani-admin-plugin.com/mani_admin_plugin/$build_wildcard/";

my $kb = sprintf("%d", $bsize / 1024);

if ($^O eq "MSWin32")
{
	print "Windows platform\n";
	$dev_base=$windows_dev_base;
}
else
{
	#Linux platform
	print "Linux platform\n";
	$dev_base=$linux_dev_base;
}

#Setup source location for the zip files to be uploaded
$upload_source=$dev_base . "/public_build";

print "Host name $host_name\n";
print "Copying from $upload_source\ with a wildcard of $build_wildcard\n";
print "A hash mark represents transfer of $kb kB\n";

opendir MYDIR, "$upload_source/";
@contents = grep !/^\.\.?$/, readdir MYDIR;
closedir MYDIR;

@http_list = ();

foreach $listitem ( @contents )
{
	# Make sure it's a zip file and has our build version
	if (grep(/\.zip$/, $list_item) &&
	    grep($build_wildcard, $list_item))
	{
		if ( -e $upload_source . "/" . $listitem)
		{
			transfer_file($listitem);	
		}
	}
}

print "HTTP URLs\n";
foreach $http_link ( @http_list )
{
	print "$http_link\n";
}
exit 0;

########################
# functions
########################

sub transfer_file
{
$ftp = Net::FTP->new ($host_name) or die "Cannot connect to $host\n";
$ftp -> login($user_login, $user_pass) or die "Cannot login to $host\n";
$ftp -> cwd($upload_target) or die "Cannot change to $upload_target\n";
$ftp -> binary;
$ftp -> hash(1, $bsize);
print "Transferring file $_[0].....\n";
$ftp -> delete ("upload.LCK");
$ftp -> put ($upload_source . "/" . $_[0], "upload.LCK") or die "Failed to upload $_[0]\n";
$ftp -> delete ($_[0]);
$ftp -> rename ("upload.LCK", $_[0]);
$ftp -> quit;
push(@http_list, $http_url . $_[0]);
}
