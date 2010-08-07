#!/usr/bin/perl

use Cwd;
use File::Basename;
use File::Spec::Functions qw(rel2abs);

# Starts up an instance of a Valve game

sub execute_game;
$BASE_FOLDER=dirname(rel2abs($0)) . "/..";

$DEFAULT_PORT="27100";

if ($^O eq "MSWin32")
{
	print "Windows platform\n";
	$ROOT_PATH=$BASE_FOLDER;
	$GAME_EXE="start /abovenormal /wait srcds.exe -console";
}
else
{
#Linux platform
	print "Linux platform\n";
	$LINUX="TRUE";
	$ROOT_PATH=$BASE_FOLDER;
	$GAME_EXE="./srcds_run -console";	
}

$SRCDS_PATH="$ROOT_PATH/srcds_1";

#Add new games here with their Valve game names
%game_cmd = (
"Counter-Strike Source"		=> "-game cstrike -tickrate 66 -secure +map de_dust +maxplayers 32 +mp_dynamicpricing 0",
"Half-Life 2 Deathmatch" 	=> "-game hl2mp -secure +map dm_lockdown +maxplayers 16",
"Team Fortress 2" 		=> "-game tf -secure +map cp_dustbowl +maxplayers 16",
"Day of Defeat Source" 		=> "-game dod -secure +map dod_donner +maxplayers 32",
"Dystopia" 			=> "-game dystopia -secure +map dys_vaccine +maxplayers 16",
"Empires" 			=> "-game empires -secure +map emp_canyon +maxplayers 16",
"The Hidden" 			=> "-game hidden -secure +map hdn_executive +maxplayers 16",
"HL2 Assault" 			=> "-game hl2as -secure +map as_citytension +maxplayers 16",
"HL2 CTF" 			=> "-game hl2ctf -secure +map ctf_defrost +maxplayers 16",
"Sourceforts" 			=> "-game sourceforts -secure +map sf_magma +maxplayers 32",
"Counter-Strike Source Beta"		=> "-game cstrike_beta -tickrate 66 -secure +map de_dust +maxplayers 32"
);

%extra_game_path = (
"Team Fortress 2" 		=> "/orangebox",
"Day of Defeat Source"		=> "/orangebox",
"Counter-Strike Source"	=> "/orangebox",
"Counter-Strike Source Beta"	=> "/cssbeta"
);

print "\n";

while(1)
{

if ($LINUX)
{
	system("stty sane");
}

$count=1;
for my $key ( sort keys %game_cmd ) 
{
	my $value = $game_cmd{$key};
        print "$count - $key\n";
        $count = $count + 1;
}

print "\nChoose a game to start\n\n";

$question_response = <>;
chomp($question_response);


$count=1;
$found=0;
for my $key ( sort keys %game_cmd ) 
{
	if ($count eq $question_response)
	{
		print "Perl script - Starting Game $key\n";
		my $value = $game_cmd{$key};
		my $path_extension = $extra_game_path{$key};
		execute_game($value, $path_extension);
		$found=1;
	}	        
        $count = $count + 1;
}	

if ($found eq 0)
{
	exit 0;
}

print "\n\n";
}

sub execute_game
{
	$system_command="$GAME_EXE $_[0]";
	$this_game_path = "$SRCDS_PATH$_[1]";
	chdir("$this_game_path");
	print "$this_game_path/$system_command\n\n";
	print "$system_command\n";
	$dir = getcwd();
	print "Current path = $dir\n";
	system($system_command);
}

