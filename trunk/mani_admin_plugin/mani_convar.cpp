//
// Mani Admin Plugin
//
// Copyright © 2009-2015 Giles Millward (Mani). All rights reserved.
//
// This file is part of ManiAdminPlugin.
//
// Mani Admin Plugin is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Mani Admin Plugin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Mani Admin Plugin.  If not, see <http://www.gnu.org/licenses/>.
//


//




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <convar.h>

ConVar mani_path( "mani_path", "mani_admin_plugin", 0, "This is the path after /cstrike/cfg/ that the mani admin plugin config files are stored" );	

ConVar mani_adverts( "mani_adverts", "1", 0, "This defines whether the adverts are on or off", true, 0, true, 1 );	
ConVar mani_time_between_adverts( "mani_time_between_adverts", "120", 0, "This defines the time between adverts", true,20, true, 1000 );	
ConVar mani_advert_dead_only( "mani_advert_dead_only", "0", 0, "Setup if you only want to show to only dead players or all players (0 = all, 1 = dead)",true, 0, true, 1);
ConVar mani_advert_col_blue( "mani_advert_col_blue", "255", 0, "This defines the blue component of the adverts (0 - 255)",true, 0, true, 255 );
ConVar mani_advert_col_red( "mani_advert_col_red", "0", 0, "This defines the red component of the adverts (0 - 255)",true, 0, true, 255 );
ConVar mani_advert_col_green( "mani_advert_col_green", "0", 0, "This defines the green component of the adverts (0 - 255)",true, 0, true, 255 );
ConVar mani_adverts_top_left ("mani_adverts_top_left", "1", 0, "This defines whether advert are shown in the top left corner of the screen (1 = on)",true, 0, true, 1 ); 
ConVar mani_adverts_chat_area ("mani_adverts_chat_area", "1", 0, "This defines whether advert are shown in the chat area of the screen (1 = on)",true, 0, true, 1 ); 
ConVar mani_adverts_bottom_area ("mani_adverts_bottom_area", "1", 0, "This defines whether advert are shown in the bottom area of the screen where hints are shown (1 = on)",true, 0, true, 1 ); 
ConVar mani_hint_sounds ("mani_hint_sounds", "0", 0, "This turns on and off the sounds related to hint messages on Orange Box games", true, 0, true, 1);

ConVar mani_tk_protection( "mani_tk_protection", "1", 0, "This defines whether the tk protection is enabled",true, 0, true, 1 );	
ConVar mani_tk_forgive( "mani_tk_forgive", "1", 0, "This defines whether the tk forgive menu is on or off",true, 0, true, 1 );	
ConVar mani_tk_spawn_time( "mani_tk_spawn_time", "5", 0, "This defines how int after the freeze time that spawn protection is enabled",true, 0, true, 120 );	
ConVar mani_tk_offences_for_ban( "mani_tk_offences_for_ban", "5", 0, "This defines tk violations required before a ban is enforced for a player",true, 0, true, 100 );	
ConVar mani_tk_ban_time( "mani_tk_ban_time", "5", 0, "This defines how int a player will be banned for if they exceed tk violation limit",true, 0, true, 100000  );	
ConVar mani_tk_blind_amount( "mani_tk_blind_amount", "253", 0, "This defines how much blindness for tk blind punishment 255 = completely blind 0 = not blind",true, 0, true, 255 );
ConVar mani_tk_slap_to_damage( "mani_tk_slap_to_damage", "10", 0, "This defines how much a player can be slapped to (1 - 80)", true, 1, true, 80);
ConVar mani_tk_slap_on_team_wound( "mani_tk_slap_on_team_wound", "0", 0, "This defines whether a player should be slapped when team wounded", true, 0, true, 1);
ConVar mani_tk_slap_on_team_wound_damage( "mani_tk_slap_on_team_wound_damage", "0", 0, "This defines how much damage should be inflicted when slapped for a team wound", true, 1, true, 80);
ConVar mani_tk_cash_percent( "mani_tk_cash_percent", "30", 0, "This defines what percentage of a players cash you can take (1 - 100)", true, 1, true, 100);
ConVar mani_tk_show_opposite_team_wound( "mani_tk_show_opposite_team_wound", "1", 0, "This defines whether team wounds on the opposite team show in the say area", true, 0, true, 1 );
ConVar mani_tk_add_violation_without_forgive( "mani_tk_add_violation_without_forgive", "0", 0, "This defines whether the attackers violation count increases regardless of the victim forgiving", true, 0, true, 1 );
ConVar mani_tk_allow_forgive_option( "mani_tk_allow_forgive_option", "1", 0, "This defines whether the tk forgive option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_slay_option( "mani_tk_allow_slay_option", "1", 0, "This defines whether the tk slay option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_blind_option( "mani_tk_allow_blind_option", "1", 0, "This defines whether the tk blind option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_slap_option( "mani_tk_allow_slap_option", "1", 0, "This defines whether the tk slap option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_freeze_option( "mani_tk_allow_freeze_option", "1", 0, "This defines whether the tk freeze option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_cash_option( "mani_tk_allow_cash_option", "1", 0, "This defines whether the tk cash option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_drugged_option( "mani_tk_allow_drugged_option", "1", 0, "This defines whether the tk drugged option can be used or not", true, 0, true, 1 );
ConVar mani_tk_allow_burn_option( "mani_tk_allow_burn_option", "1", 0, "This defines whether the tk burn option can be used or not", true, 0, true, 1 );
ConVar mani_tk_burn_time( "mani_tk_burn_time", "100", 0, "This defines how long the burn time should be for in seconds for the tk punishment", true, 10, true, 300);
ConVar mani_tk_allow_time_bomb_option( "mani_tk_allow_time_bomb_option", "1", 0, "This defines whether the tk time bomb option can be used or not", true, 0, true, 1 );
ConVar mani_tk_time_bomb_seconds( "mani_tk_time_bomb_seconds", "10", 0, "This defines the time before the bomb goes off", true, 5, true, 120 );
ConVar mani_tk_time_bomb_blast_radius ( "mani_tk_time_bomb_blast_radius", "1000", 0, "This defines the bomb blast radius", true, 50, true, 20000 );
ConVar mani_tk_time_bomb_show_beams ( "mani_tk_time_bomb_show_beams", "1", 0, "0 = no beams, 1 = beams on", true, 0, true, 1);
ConVar mani_tk_time_bomb_blast_mode ( "mani_tk_time_bomb_blast_mode", "2", 0, "0 = player only, 1 = players on same team, 2 = all players", true, 0, true, 2);
ConVar mani_tk_time_bomb_beep_radius ( "mani_tk_time_bomb_beep_radius", "256", 0, "This defines the end radius of the visual beep, 0 = radius equal bomb blast radius", true, 0, true, 2000);

ConVar mani_tk_allow_fire_bomb_option( "mani_tk_allow_fire_bomb_option", "1", 0, "This defines whether the tk fire bomb option can be used or not", true, 0, true, 1 );
ConVar mani_tk_fire_bomb_seconds( "mani_tk_fire_bomb_seconds", "10", 0, "This defines the time before the bomb goes off", true, 5, true, 120 );
ConVar mani_tk_fire_bomb_blast_radius ( "mani_tk_fire_bomb_blast_radius", "1000", 0, "This defines the bomb blast radius", true, 50, true, 20000 );
ConVar mani_tk_fire_bomb_show_beams ( "mani_tk_fire_bomb_show_beams", "1", 0, "0 = no beams, 1 = beams on", true, 0, true, 1);
ConVar mani_tk_fire_bomb_blast_mode ( "mani_tk_fire_bomb_blast_mode", "2", 0, "0 = player only, 1 = players on same team, 2 = all players", true, 0, true, 2);
ConVar mani_tk_fire_bomb_burn_time ( "mani_tk_fire_bomb_burn_time", "100", 0, "Time in seconds that players will burn for", true, 10, true, 300);
ConVar mani_tk_fire_bomb_beep_radius ( "mani_tk_fire_bomb_beep_radius", "256", 0, "This defines the end radius of the visual beep, 0 = radius equal bomb blast radius", true, 0, true, 2000);

ConVar mani_tk_allow_freeze_bomb_option( "mani_tk_allow_freeze_bomb_option", "1", 0, "This defines whether the tk freeze bomb option can be used or not", true, 0, true, 1 );
ConVar mani_tk_freeze_bomb_seconds( "mani_tk_freeze_bomb_seconds", "10", 0, "This defines the time before the bomb goes off", true, 5, true, 120 );
ConVar mani_tk_freeze_bomb_blast_radius ( "mani_tk_freeze_bomb_blast_radius", "1000", 0, "This defines the bomb blast radius", true, 50, true, 20000 );
ConVar mani_tk_freeze_bomb_show_beams ( "mani_tk_freeze_bomb_show_beams", "1", 0, "0 = no beams, 1 = beams on", true, 0, true, 1);
ConVar mani_tk_freeze_bomb_blast_mode ( "mani_tk_freeze_bomb_blast_mode", "2", 0, "0 = player only, 1 = players on same team, 2 = all players", true, 0, true, 2);
ConVar mani_tk_freeze_bomb_beep_radius ( "mani_tk_freeze_bomb_beep_radius", "256", 0, "This defines the end radius of the visual beep, 0 = radius equal bomb blast radius", true, 0, true, 2000);

ConVar mani_tk_allow_beacon_option( "mani_tk_allow_beacon_option", "1", 0, "This defines whether the tk beacon option can be used or not", true, 0, true, 1 );
ConVar mani_tk_beacon_radius ( "mani_tk_beacon_radius", "384", 0, "This defines the end radius of the visual beep", true, 20, true, 2000);

ConVar mani_tk_allow_bots_to_punish( "mani_tk_allow_bots_to_punish", "1", 0, "This defines whether bots can punish or not", true, 0, true, 1 );
ConVar mani_tk_allow_bots_to_add_violations( "mani_tk_allow_bots_to_add_violations", "1", 0, "This defines whether bots can add to players tk violations", true, 0, true, 1 );

ConVar mani_tk_team_wound_reflect ( "mani_tk_team_wound_reflect", "1", 0, "This defines whether reflective team wounding is turned on or off", true, 0, true, 1 );
ConVar mani_tk_team_wound_reflect_threshold ( "mani_tk_team_wound_reflect_threshold", "5", 0, "This defines how many team wounds are allowed before reflection damage is used", true, 0, true, 999);
ConVar mani_tk_team_wound_reflect_ratio ( "mani_tk_team_wound_reflect_ratio", "1.0", 0, "This defines the start reflective damage ratio to be applied to the attacker", true, 0.1, true, 100);
ConVar mani_tk_team_wound_reflect_ratio_increase ( "mani_tk_team_wound_reflect_ratio_increase", "0.2", 0, "This defines the amount to add on to each players current reflection ratio per each team wound", true, 0, true, 999);

ConVar mani_adminsay_anonymous( "mani_adminsay_anonymous", "0", 0, "This defines whether admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminkick_anonymous( "mani_adminkick_anonymous", "0", 0, "This defines whether kick admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminslay_anonymous( "mani_adminslay_anonymous", "0", 0, "This defines whether slay admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminslap_anonymous( "mani_adminslap_anonymous", "0", 0, "This defines whether slap admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminban_anonymous( "mani_adminban_anonymous", "0", 0, "This defines whether ban admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminblind_anonymous( "mani_adminblind_anonymous", "0", 0, "This defines whether blind admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminfreeze_anonymous( "mani_adminfreeze_anonymous", "0", 0, "This defines whether freeze admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admindrug_anonymous( "mani_admindrug_anonymous", "0", 0, "This defines whether drug admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminburn_anonymous( "mani_adminburn_anonymous", "0", 0, "This defines whether burn admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminteleport_anonymous( "mani_adminteleport_anonymous", "0", 0, "This defines whether teleport admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminvote_anonymous( "mani_adminvote_anonymous", "0", 0, "This defines whether admins who start votes names are shown to the public",true, 0, true, 1 );
ConVar mani_adminnoclip_anonymous( "mani_adminnoclip_anonymous", "0", 0, "This defines whether noclip admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminmute_anonymous( "mani_adminmute_anonymous", "0", 0, "This defines whether mute admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admincash_anonymous( "mani_admincash_anonymous", "0", 0, "This defines whether cash admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminsetskin_anonymous( "mani_adminsetskin_anonymous", "0", 0, "This defines whether setskin admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admindropc4_anonymous( "mani_admindropc4_anonymous", "0", 0, "This defines whether drop c4 admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admintimebomb_anonymous( "mani_admintimebomb_anonymous", "0", 0, "This defines whether time bomb admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminfirebomb_anonymous( "mani_adminfirebomb_anonymous", "0", 0, "This defines whether fire bomb admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminfreezebomb_anonymous( "mani_adminfreezebomb_anonymous", "0", 0, "This defines whether freeze bomb admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminbeacon_anonymous( "mani_adminbeacon_anonymous", "0", 0, "This defines whether beacon admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminhealth_anonymous( "mani_adminhealth_anonymous", "0", 0, "This defines whether sethealth admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admingive_anonymous( "mani_admingive_anonymous", "0", 0, "This defines whether give item admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admincolor_anonymous( "mani_admincolor_anonymous", "0", 0, "This defines whether color admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admingravity_anonymous( "mani_admingravity_anonymous", "0", 0, "This defines whether gravity admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );

ConVar mani_adminmap_anonymous( "mani_adminmap_anonymous", "0", 0, "This defines whether ma_nextmap admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_adminswap_anonymous( "mani_adminswap_anonymous", "0", 0, "This defines whether ma_swapteam admin messages are anonymous to non admins (1 = anonymous)",true, 0, true, 1 );
ConVar mani_admin_burn_time( "mani_admin_burn_time", "20", 0, "This defines how long the burn time should be for in seconds when triggered by admin", true, 10, true, 300);

ConVar mani_adminsay_top_left ("mani_adminsay_top_left", "1", 0, "This defines whether adminsay is shown in the top left corner of the screen (1 = on)",true, 0, true, 1 ); 
ConVar mani_adminsay_chat_area ("mani_adminsay_chat_area", "1", 0, "This defines whether adminsay is shown in the chat area of the screen (1 = on)",true, 0, true, 1 ); 
ConVar mani_adminsay_bottom_area ("mani_adminsay_bottom_area", "1", 0, "This defines whether adminsay is shown in the bottom area of the screen (1 = on)",true, 0, true, 1 ); 

ConVar mani_stats ("mani_stats", "1", 0, "This defines whether stats are turned on (1 = on)",true, 0, true, 1 ); 
ConVar mani_stats_mode ("mani_stats_mode", "1", 0, "This defines when the stats are re-caclulated, 0 = at map load, 1 = per new round",true, 0, true, 1 ); 
ConVar mani_stats_calculate ("mani_stats_calculate", "3", 0, "This defines how the stats are caclulated, 0 = by player kills, 1 = by player kill death ratio, 2 = by kills - deaths, 3 = by points",true, 0, true, 3 ); 
ConVar mani_stats_drop_player_days ("mani_stats_drop_player_days", "365", 0, "This defines how many days should pass before a player is dropped from the stats for not playing",true, 1, true, 365 ); 
ConVar mani_stats_kills_required ("mani_stats_kills_required", "20", 0, "This defines how many kills are required before a player is shown in the command rank and top ",true, 0, true, 50000 ); 
ConVar mani_stats_top_display_time ("mani_stats_top_display_time", "10", 0, "This defines the time that the 'top' information displays for 5 - 30 seconds",true, 5, true, 30 ); 
ConVar mani_stats_show_rank_to_all ("mani_stats_show_rank_to_all", "1", 0, "This defines whether the command 'rank' is shown to all or not (1 = all players)",true, 0, true, 1 ); 
ConVar mani_stats_alternative_rank_message ("mani_stats_alternative_rank_message", "", 0, "This defines the message to be displayed to the user when rank is typed if stats are turned off"); 
ConVar mani_stats_write_text_file ("mani_stats_write_text_file", "1", 0, "This defines whether the ranks are written to text file (1 = yes)", true, 0, true, 1 ); 
ConVar mani_stats_calculate_frequency ("mani_stats_calculate_frequency", "0", 0, "This defines how often the stats should be recalculated in minutes (default is 0)", true, 0, true, 86400 ); 
ConVar mani_stats_write_frequency_to_disk ("mani_stats_write_frequency_to_disk", "0", 0, "This defines how often the stats should be recalculated AND written to disk in minutes (default is 0)", true, 0, true, 86400 ); 
ConVar mani_stats_include_bot_kills ("mani_stats_include_bot_kills", "0", 0, "0 = Bot kills are not counted, 1 = Bot kills are counted", true, 0, true, 1 ); 
ConVar mani_stats_most_destructive ("mani_stats_most_destructive", "0", 0, "0 = Disable most destructive display, 1 = Enable most destructive display", true, 0, true, 1); 

ConVar mani_ff_player_only ("mani_ff_player_only", "0", 0, "This defines whether the whole server sees the message or not (0 = whole server)",true, 0, true, 1 ); 
ConVar mani_nextmap_player_only ("mani_nextmap_player_only", "0", 0, "This defines whether the whole server sees the message or not (0 = whole server)",true, 0, true, 1 ); 
ConVar mani_timeleft_player_only ("mani_timeleft_player_only", "0", 0, "This defines whether the whole server sees the message or not (0 = whole server)",true, 0, true, 1 ); 
ConVar mani_thetime_player_only ("mani_thetime_player_only", "0", 0, "This defines whether the whole server sees the message or not (0 = whole server)",true, 0, true, 1 ); 

ConVar mani_war_mode_force_overview_zero ("mani_war_mode_force_overview_zero", "0", 0, "This defines whether overview_mode is forced to 0 for dead players each game frame (1 = force overview mode to 0)", true, 0, true, 1);

ConVar mani_reserve_slots ("mani_reserve_slots", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "This defines whether reserve slots are on or off (1 = on)", true, 0, true, 1); 
ConVar mani_reserve_slots_number_of_slots ("mani_reserve_slots_number_of_slots", "0", 0, "This defines how many reserve slots you have", true, 0, true, 128); 
ConVar mani_reserve_slots_kick_message ("mani_reserve_slots_kick_message", "", 0, "This defines the message displayed in the client console when kicked for taking up a reserve slot"); 
ConVar mani_reserve_slots_redirect_message ("mani_reserve_slots_redirect_message", "", 0, "This defines the message displayed in the client console when redirected for taking up a reserve slot"); 
ConVar mani_reserve_slots_redirect ("mani_reserve_slots_redirect", "", 0, "This defines another server to redirect the player to"); 
ConVar mani_reserve_slots_allow_slot_fill ("mani_reserve_slots_allow_slot_fill", "0", 0, "This defines whether reserved slots can be used up when joining or whether the reserved slots must remain free causing player kicks instead", true, 0, true, 1); 
ConVar mani_reserve_slots_kick_method ("mani_reserve_slots_kick_method", "0", 0, "This defines how a player is kicked, 0 = by ping, 1 = by connection time, 2 = by kills per minute, 3 = kill/death ratio", true, 0, true, 3); 
ConVar mani_reserve_slots_include_admin ("mani_reserve_slots_include_admin", "1", 0, "This defines whether the admins setup are part of the reserve slot list (1 = yes they are)", true, 0, true, 1); 
ConVar mani_reserve_slots_ip_keep_history ( "mani_reserve_slots_ip_keep_history", "14", 0, "The number of days to keep players IP history for reserve slots", true, 1, true, 31 );
ConVar mani_reserve_slots_enforce_password ( "mani_reserve_slots_enforce_password", "0", 0, "This enforces the server password to be turned on even for administrators.", true, 0, true, 1 );

ConVar mani_reverse_admin_flags ("mani_reverse_admin_flags", "0", 0, "Set to 1 if you want admin flags to be reversed in meaning (default = 0)", true, 0, true, 1); 
ConVar mani_reverse_immunity_flags ("mani_reverse_immunity_flags", "0", 0, "Set to 1 if you want immunity flags to be reversed in meaning (default = 0)", true, 0, true, 1); 

ConVar mani_military_time ("mani_military_time", "1", 0, "Set to 1 if you want the plugin to show military time, 0 for normal 12 hour clock", true, 0, true, 1); 

ConVar mani_chat_flood_time ("mani_chat_flood_time", "0", 0, "Amount of time before spam chat kicks in", true, 0, true, 15); 
ConVar mani_chat_flood_message ("mani_chat_flood_message", "STOP SPAMMING THE SERVER !!", 0, "Message to be displayed if the player spams the server"); 

ConVar mani_command_flood_time ("mani_command_flood_time", "1", 0, "Amount of time to check for command spamming ( 0 = off )", true, 0, true, 4);
ConVar mani_command_flood_total ("mani_command_flood_total", "14", 0, "Number of commands allowed in the set time to allow" );

//birkett - added command spamming punishment
ConVar mani_command_flood_punish ("mani_command_flood_punish", "0", 0, "Punish players for command spamming. 0 = Disabled, 1 = Kick player, 2 = Ban player");
ConVar mani_command_flood_violation_count ("mani_command_flood_violation_count", "5", 0, "Number of times a player goes past the limit before being punished");
ConVar mani_command_flood_punish_ban_time ("mani_command_flood_punish_ban_time", "60", 0, "Time in minutes to ban for after chat spamming");

ConVar mani_use_ma_in_say_command ("mani_use_ma_in_say_command", "0", 0, "If 0 then you don't need to prepend ma_ to say commands, if 1 then you do (Beetle compatabilty) ", true, 0, true, 1); 

ConVar mani_sounds_per_round ("mani_sounds_per_round", "0", 0, "Number of sounds a regulary player can play in the course of a round", true, 0, true, 100); 
ConVar mani_sounds_filter_if_dead ("mani_sounds_filter_if_dead", "0", 0, "1 = If a player is dead then only other dead players will hear it", true, 0, true, 1); 
ConVar mani_show_victim_stats ("mani_show_victim_stats", "0", 0, "Shows victim stats for each player when they die if set to 1", true, 0, true, 1); 
ConVar mani_show_victim_stats_inflicted_only ("mani_show_victim_stats_inflicted_only", "0", 0, "If set to 1 only shows players you hurt or killed", true, 0, true, 1); 
ConVar mani_play_sound_type ( "mani_play_sound_type", "0", 0, "The method sounds are played to the client.  0 = playgamesound, 1 = play", true, 0, true, 1);


ConVar mani_use_amx_style_menu ("mani_use_amx_style_menu", "1", 0, "If set to 1 use amx style menus, if 0 uses Escape style menus", true, 0, true, 1); 
ConVar mani_autobalance_teams ("mani_autobalance_teams", "0", 0, "If set to 1 autobalancing is performed at the end of each round", true, 0, true, 1); 
ConVar mani_autobalance_mode ("mani_autobalance_mode", "1", 0, "0 = Players balanced regardless if dead or alive, 1 = dead players swapped first followed by alive players, 2 = only dead players can be swapped", true, 0, true, 2); 
ConVar mani_thetime_timezone ("mani_thetime_timezone", " ", 0, "The text set here is appended to the time when displayed"); 

ConVar mani_voting( "mani_voting", "1", 0, "This defines whether the voting system is on or off",true, 0, true, 1 );	
ConVar mani_vote_allow_end_of_map_vote ("mani_vote_allow_end_of_map_vote", "0", 0, "Defines whether a random map vote will be displayed towards the end of the map", true, 0, true, 1); 
ConVar mani_vote_allow_extend ("mani_vote_allow_extend", "1", 0, "Defines the whether the a map can be extended", true, 0, true, 1); 
ConVar mani_vote_allow_user_vote_ban("mani_vote_allow_user_vote_ban", "0", 0, "0 = off, 1 = users can type voteban to vote to ban a player", true, 0, true, 1); 
ConVar mani_vote_allow_user_vote_ban_ghost("mani_vote_allow_user_vote_ban_ghost", "1", 0, "0 = off, 1 = users sharing an ip address can type voteban to vote to ban a player", true, 0, true, 1); 
ConVar mani_vote_allow_user_vote_kick ("mani_vote_allow_user_vote_kick", "0", 0, "0 = off, 1 = users can type votekick to vote to kick a player", true, 0, true, 1); 
ConVar mani_vote_allow_user_vote_kick_ghost ("mani_vote_allow_user_vote_kick_ghost", "1", 0, "0 = off, 1 = users sharing an ip address can kick players", true, 0, true, 1); 
ConVar mani_vote_allow_user_vote_map ("mani_vote_allow_user_vote_map", "0", 0, "0 = off, 1 = users can type votemap to vote for a map", true, 0, true, 1); 
ConVar mani_vote_allow_user_vote_map_extend ("mani_vote_allow_user_vote_map_extend", "0", 0, "0 = off, 1 = users can extend on a vote map", true, 0, true, 1); 
ConVar mani_vote_allowed_voting_time ("mani_vote_allowed_voting_time", "45", 0, "Defines amount of time in seconds a vote will be allowed for", true, 10, true, 90); 

ConVar mani_vote_dont_show_if_alive ("mani_vote_dont_show_if_alive", "0", 0, "0 = alive players will see vote menu, 1 = alive players will need to type vote to access the menu", true, 0, true, 1); 
ConVar mani_vote_dont_show_last_maps ("mani_vote_dont_show_last_maps", "3", 0, "Defines the last number of maps played to not show in random votemap lists", true, 0, true, 20); 
ConVar mani_vote_end_of_map_percent_required ("mani_vote_end_of_map_percent_required", "60", 0, "Defines the vote percentage required to set map", true, 0, true, 100); 
ConVar mani_vote_extend_time ("mani_vote_extend_time", "20", 0, "Defines the time in minutes a extend vote will add to the timeleft counter", true, 5, true, 99999); 
ConVar mani_vote_extend_percent_required ("mani_vote_extend_percent_required", "60", 0, "Defines the vote percentage required to set an extend map vote", true, 0, true, 100); 
ConVar mani_vote_extend_rounds ("mani_vote_extend_rounds", "10", 0, "Defines the number of rounds to extend mp_winlimit by", true, 2, true, 99999); 
ConVar mani_vote_map_percent_required ("mani_vote_map_percent_required", "60", 0, "Defines the vote percentage required to set map vote", true, 0, true, 100); 
ConVar mani_vote_mapcycle_mode_for_admin_map_vote ("mani_vote_mapcycle_mode_for_admin_map_vote", "0", 0, "0 = mapcycle.txt, 1 = votemapslist.txt, 2 = maplist.txt", true, 0, true, 2); 
ConVar mani_vote_mapcycle_mode_for_random_map_vote ("mani_vote_mapcycle_mode_for_random_map_vote", "0", 0, "0 = mapcycle.txt, 1 = votemapslist.txt, 2 = maplist.txt", true, 0, true, 2); 
ConVar mani_vote_max_extends ("mani_vote_max_extends", "2", 0, "Defines how many times a map can be extended via random vote, 0 = infinite", true, 0, true, 99999); 
ConVar mani_vote_max_maps_for_end_of_map_vote ("mani_vote_max_maps_for_end_of_map_vote", "6", 0, "Defines how many maps can be in the end of map vote", true, 0, true, 99999); 
ConVar mani_vote_question_percent_required ("mani_vote_question_percent_required", "60", 0, "Defines the vote percentage required to set question vote", true, 0, true, 100); 
ConVar mani_vote_random_map_percent_required ("mani_vote_random_map_percent_required", "60", 0, "Defines the vote percentage required to set random map vote", true, 0, true, 100); 
ConVar mani_vote_rcon_percent_required ("mani_vote_rcon_percent_required", "60", 0, "Defines the vote percentage required to set rcon vote", true, 0, true, 100); 
ConVar mani_vote_rounds_before_end_of_map_vote ("mani_vote_rounds_before_end_of_map_vote", "3", 0, "Defines how rounds before mp_winlimit is hit that a random map vote is started", true, 3, true, 99999); 
ConVar mani_vote_show_vote_mode ("mani_vote_show_vote_mode", "3", 0, "0 = quiet mode, 1 = show players as they vote but not their choice, 2 = Show voted choice but not player, 3 = show player name and their choice", true, 0, true, 3); 
ConVar mani_vote_time_before_end_of_map_vote ("mani_vote_time_before_end_of_map_vote", "3", 0, "Defines how many minutes before the end of the map that a random map vote is started", true, 2, true, 99999); 
ConVar mani_vote_randomize_extend_vote ("mani_vote_randomize_extend_vote", "0",0, "Randomize slot number the option \"Extend Map\" will be displayed", true, 0, true, 1);
ConVar mani_vote_end_of_map_swap_team ("mani_vote_end_of_map_swap_team", "0", 0, "Defines if teams are swapped on an extend vote win for end of map vote", true, 0, true, 1); 

ConVar mani_vote_user_vote_ban_minimum_votes( "mani_vote_user_vote_ban_minimum_votes", "4", 0, "This defines the minimum amount of votes required from players to ban a player",true, 0, true, 64 );	
ConVar mani_vote_user_vote_ban_mode ("mani_vote_user_vote_ban_mode", "0", 0, "0 = only when no admin on server, 1 = all the time", true, 0, true, 1); 
ConVar mani_vote_user_vote_ban_percentage( "mani_vote_user_vote_ban_percentage", "60", 0, "This defines the required percentage of players votes that are needed to ban a player",true, 0, true, 100  );	
ConVar mani_vote_user_vote_ban_time_before_vote( "mani_vote_user_vote_ban_time_before_vote", "60", 0, "This defines the amount of time before voting is allowed after a map change",true, 0, true, 10000   );	
ConVar mani_vote_user_vote_ban_time( "mani_vote_user_vote_ban_time", "30", 0, "0 = permanent ban, > 0 in minutes",true, 0, true, 99999999);	
ConVar mani_vote_user_vote_ban_type( "mani_vote_user_vote_ban_type", "0", 0, "0 = ban by ID, 1 = ban by IP, 2 = ban by ID and IP", true, 0, true, 2);	

ConVar mani_vote_user_vote_map_minimum_votes( "mani_vote_user_vote_map_minimum_votes", "4", 0, "This defines the minimum amount of votes required from players to change map",true, 0, true, 64 );	
ConVar mani_vote_user_vote_map_percentage( "mani_vote_user_vote_map_percentage", "60", 0, "This defines the required percentage of players votes that are needed to change a map",true, 0, true, 100  );	
ConVar mani_vote_user_vote_map_time_before_vote( "mani_vote_user_vote_map_time_before_vote", "60", 0, "This defines the amount of time before voting is allowed after a map change",true, 0, true, 10000   );	

ConVar mani_vote_user_vote_kick_minimum_votes( "mani_vote_user_vote_kick_minimum_votes", "4", 0, "This defines the minimum amount of votes required from players to kick a player",true, 0, true, 64 );	
ConVar mani_vote_user_vote_kick_mode ("mani_vote_user_vote_kick_mode", "0", 0, "0 = only when no admin on server, 1 = all the time", true, 0, true, 1); 
ConVar mani_vote_user_vote_kick_percentage( "mani_vote_user_vote_kick_percentage", "60", 0, "This defines the required percentage of players votes that are needed to kick a player",true, 0, true, 100  );	
ConVar mani_vote_user_vote_kick_time_before_vote( "mani_vote_user_vote_kick_time_before_vote", "60", 0, "This defines the amount of time before voting is allowed after a map change",true, 0, true, 10000   );	


ConVar mani_vote_allow_rock_the_vote ("mani_vote_allow_rock_the_vote", "0", 0, "0 = off, 1 = users can type rock the vote", true, 0, true, 1); 
ConVar mani_vote_rock_the_vote_threshold_percent ("mani_vote_rock_the_vote_threshold_percent", "50", 0, "Percentage of human players required to start rock the vote", true, 0, true, 100); 
ConVar mani_vote_rock_the_vote_threshold_minimum ("mani_vote_rock_the_vote_threshold_minimum", "3", 0, "This defines the minimum amount of votes required from players to start rock the vote",true, 0, true, 64 );	
ConVar mani_vote_rock_the_vote_percent_required ("mani_vote_rock_the_vote_percent_required", "60", 0, "Defines the vote percentage required to complete a rock the vote type vote", true, 0, true, 100); 
ConVar mani_vote_time_before_rock_the_vote ("mani_vote_time_before_rock_the_vote", "0", 0, "Time in minutes before rock the vote is allowed", true, 0, true, 99999); 
ConVar mani_vote_rock_the_vote_number_of_nominations ("mani_vote_rock_the_vote_number_of_nominations", "2", 0, "0 = players cannot nominate a map before rock the vote, > 0 is the number of maps added to the vote", true, 0, true, 1); 
ConVar mani_vote_rock_the_vote_number_of_maps ("mani_vote_rock_the_vote_number_of_maps", "6", 0, "total number of maps shown (including nominations)", true, 0, true, 9999); 

ConVar mani_player_name_change_threshold ("mani_player_name_change_threshold", "15", 0, "0 = off otherwise set to the number of name changes allowed before action is taken", true, 0, true, 1000); 
ConVar mani_player_name_change_reset ("mani_player_name_change_reset", "0", 0, "0 = reset name change count per round, 1 = reset name change count per map", true, 0, true, 1); 
ConVar mani_player_name_change_punishment ("mani_player_name_change_punishment", "0", 0, "0 = kick, 1 = ban by ID, 2 = ban by IP, 3 = ban by ID and IP", true, 0, true, 3); 
ConVar mani_player_name_change_ban_time ("mani_player_name_change_ban_time", "0", 0, "0 = permanent ban otherwise specifies the number of minutes", true, 0, true, 9999999); 

//ConVar mani_player_cheat_punishment ("mani_player_cheat_punishment", "0", 0, "0 = kick, 1 = ban by ID, 2 = ban by IP, 3 = ban by ID and IP", true, 0, true, 3); 
//ConVar mani_player_cheat_ban_time ("mani_player_cheat_ban_time", "0", 0, "0 = permanent ban otherwise specifies the number of minutes", true, 0, true, 9999999); 

ConVar mani_allow_chat_to_admin ("mani_allow_chat_to_admin", "1", 0, "1 = allows users to chat to admin", true, 0, true, 1); 


ConVar mani_filter_words_mode ("mani_filter_words_mode", "0", 0, "0 = off, 1 = show warning to player and block chat", true, 0, true, 1); 
ConVar mani_filter_words_warning ("mani_filter_words_warning", "SWEARING IS NOT ALLOWED ON THIS SERVER !!!", 0, "Phrase to say to player if warning mode is enabled"); 

ConVar mani_adjust_time ("mani_adjust_time", "0", 0, "Adjust time shown in minutes", true, -1440, true, 1440); 

ConVar mani_player_settings_quake ("mani_player_settings_quake", "0", 0, "0 = player settings default to off, 1 = player settings default to on", true, 0, true, 1); 
ConVar mani_player_settings_sounds ("mani_player_settings_sounds", "1", 0, "0 = player settings default to off, 1 = player settings default to on", true, 0, true, 1); 
ConVar mani_player_settings_damage ("mani_player_settings_damage", "0", 0, "0 = player settings default to off, 1 = player settings default to mode 1, 2 = player settings to mode 2, etc up to mode 3 ", true, 0, true, 3); 
ConVar mani_player_settings_death_beam ("mani_player_settings_death_beam", "0", 0, "0 = player settings default to off, 1 = player settings default to on", true, 0, true, 1); 
ConVar mani_player_settings_destructive ("mani_player_settings_destructive", "0", 0, "0 = player settings default to off, 1 = player settings default to on", true, 0, true, 1); 
ConVar mani_player_settings_vote_progress ("mani_player_settings_vote_progress", "1", 0, "0 = player settings default to off, 1 = player settings default to on", true, 0, true, 1); 

ConVar mani_skins_admin ("mani_skins_admin", "0", 0, "0 = disallow admin skins, 1 = allow admin skins", true, 0, true, 1); 
ConVar mani_skins_reserved ("mani_skins_reserved", "0", 0, "0 = disallow reserved skins, 1 = allow reserved skins", true, 0, true, 1); 
ConVar mani_skins_public ("mani_skins_public", "0", 0, "0 = disallow public skins, 1 = allow public skins", true, 0, true, 1); 
ConVar mani_skins_force_public ("mani_skins_force_public", "0", 0, "0 = players can choose their skin, 1 = forces player to first custom skin", true, 0, true, 1); 
ConVar mani_skins_setskin_misc_only ("mani_skins_setskin_misc_only", "0", 0, "0 = the setskin command is for all models, 1 = setskin command is only for misc models", true, 0, true, 1); 
ConVar mani_skins_force_choose_on_join ("mani_skins_force_choose_on_join", "0", 0, "0 = let user type settings to change model, 1 = ask user to choose on team change, 2 = show settings menu on team change", true, 0, true, 2); 
ConVar mani_skins_random_bot_skins ("mani_skins_random_bot_skins", "0", 0, "0 = bots use default skin, 1 = bots choose random public skin", true, 0, true, 1); 

ConVar mani_show_death_beams ("mani_show_death_beams", "0", 0, "0 = Disable show beams, 1 = Enable show beams", true, 0, true, 1); 

ConVar mani_mute_con_command_spam ("mani_mute_con_command_spam", "0", 0, "0 = show con command use, 1 = mute con command use from server", true, 0, true, 1);

ConVar mani_sort_menus ("mani_sort_menus", "1", 0, "0 = do not sort menus, 1 = sort most menus into alphabetical order", true, 0, true, 1);

ConVar mani_blind_ghosters ("mani_blind_ghosters", "0", 0, "0 = ghost players on same IP not blinded when dead or spectating, 1 = Ghost players are blinded", true, 0, true, 1);

ConVar mani_map_adverts ("mani_map_adverts", "0", 0, "0 = disabled, 1 = enabled", true, 0, true, 1);
ConVar mani_map_adverts_in_war ("mani_map_adverts_in_war", "0", 0, "Allow adverts in war mode, 0 = disabled, 1 = enabled", true, 0, true, 1);

ConVar mani_spawnpoints_mode ("mani_spawnpoints_mode", "0", 0, "Extra spawnpoints mode, 0 = Disabled, 1 = Enabled (Uses spawnpoints.txt)", true, 0, true, 1);

ConVar mani_spray_tag ("mani_spray_tag", "0", 0, "Tracking of player sprays, 0 = Disabled, 1 = Enabled", true, 0, true, 1);
ConVar mani_spray_tag_block_mode ("mani_spray_tag_block_mode", "0", 0, "0 = Allow all sprays to be shown, 1 = Block all sprays from being shown", true, 0, true, 1);
ConVar mani_spray_tag_block_message ("mani_spray_tag_block_message", "Sprays are blocked on this server !!", 0, "Message shown to player when they try to do a spray and it is blocked");
ConVar mani_spray_tag_spray_duration ("mani_spray_tag_spray_duration", "120", 0, "Time in seconds allowed for each spray before track is lost", true, 10, true, 600);
ConVar mani_spray_tag_spray_distance_limit ("mani_spray_tag_spray_distance_limit", "500", 0, "Radius limit from current position in which sprays will be detected", true, 1, true, 3000);
ConVar mani_spray_tag_spray_highlight ("mani_spray_tag_spray_highlight", "1", 0, "Highlights the spray that the system thinks you are referring to", true, 0, true, 2);
ConVar mani_spray_tag_ban_time ("mani_spray_tag_ban_time", "60", 0, "Time in minutes that a player will be banned for", true, 1, true, 86400);
ConVar mani_spray_tag_warning_message ("mani_spray_tag_warning_message", "Please stop using your spray", 0, "This phrase is said to player when using Spray Tag tracking warning option"); 
ConVar mani_spray_tag_kick_message ("mani_spray_tag_kick_message", "You have been kicked for using an offensive spray", 0, "This phrase is said to player when using Spray Tag tracking kick option"); 
ConVar mani_spray_tag_ban_message ("mani_spray_tag_ban_message", "You have been banned for 60 minutes through using an offensive spray", 0, "This phrase is said to player when using Spray Tag tracking ban option"); 
ConVar mani_spray_tag_perm_ban_message ("mani_spray_tag_perm_ban_message", "You have been permanently banned for using an offensive spray", 0, "This phrase is said to player when using Spray Tag tracking permanent ban option"); 
ConVar mani_spray_tag_slap_damage ("mani_spray_tag_slap_damage", "0", 0, "Slap damage given to play for spray slap warn (0-100)", true, 0, true, 100);

ConVar mani_external_stats_log ("mani_external_stats_log", "0", 0, "1 = enable external stats logging, 0 = disable", true, 0, true, 1);
ConVar mani_external_stats_log_allow_war_logs ("mani_external_stats_log_allow_war_logs", "0", 0, "1 = enable external stats logging whilst in war mode, 0 = disable stats in war mode", true, 0, true, 1);

ConVar mani_hostage_follow_warning ("mani_hostage_follow_warning", "0", 0, "1 = player will be warned in console if a hostage stops following on CSS, 0 = disable warning", true, 0, true, 1);

ConVar mani_all_see_ma_rates ("mani_all_see_ma_rates", "0", 0, "0 = Only admins can access ma_rates, 1 = Anyone can use ma_rates", true, 0, true, 1);

ConVar mani_admin_temp_ban_time_limit ("mani_admin_temp_ban_time_limit", "360", 0, "Time in minutes that an admin with only Ban access can ban for, admins with Permanent Ban access can ban for as long as they wish (default is 6 hours max)", true, 0, true, 99999999);

ConVar mani_admin_temp_mute_time_limit ("mani_admin_temp_mute_time_limit", "360", 0, "Time in minutes that an admin with only Mute access can mute for, admins with Permanent Mute access can mute for as long as they wish (default is 6 hours max)", true, 0, true, 99999999);

ConVar mani_admin_default_ban_reason ("mani_admin_default_ban_reason","", 0, "If set, this will be put in for the reason if no reason is given by the admin who bans the player." );
ConVar mani_admin_default_mute_reason ("mani_admin_default_mute_reason","", 0, "If set, this will be put in for the reason if no reason is given by the admin who mutes the player." );
