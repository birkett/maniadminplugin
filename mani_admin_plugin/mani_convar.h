//
// Mani Admin Plugin
//
// Copyright © 2009-2013 Giles Millward (Mani). All rights reserved.
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


#ifndef MANI_CONVAR_H
#define MANI_CONVAR_H

#include "convar.h"

extern ConVar mani_path;

extern ConVar mani_adverts;
extern ConVar mani_time_between_adverts;
extern ConVar mani_advert_dead_only;
extern ConVar mani_advert_col_blue;
extern ConVar mani_advert_col_red;
extern ConVar mani_advert_col_green;
extern ConVar mani_adverts_top_left;
extern ConVar mani_adverts_chat_area;
extern ConVar mani_adverts_bottom_area;
extern ConVar mani_hint_sounds;

extern ConVar mani_tk_protection;
extern ConVar mani_tk_forgive;
extern ConVar mani_tk_spawn_time;
extern ConVar mani_tk_offences_for_ban;
extern ConVar mani_tk_ban_time;
extern ConVar mani_tk_blind_amount;
extern ConVar mani_tk_slap_to_damage;
extern ConVar mani_tk_slap_on_team_wound;
extern ConVar mani_tk_slap_on_team_wound_damage;
extern ConVar mani_tk_cash_percent;
extern ConVar mani_tk_show_opposite_team_wound;
extern ConVar mani_tk_add_violation_without_forgive;
extern ConVar mani_tk_allow_forgive_option;
extern ConVar mani_tk_allow_slay_option;
extern ConVar mani_tk_allow_blind_option;
extern ConVar mani_tk_allow_slap_option;
extern ConVar mani_tk_allow_freeze_option;
extern ConVar mani_tk_allow_cash_option;
extern ConVar mani_tk_allow_drugged_option;
extern ConVar mani_tk_allow_burn_option;
extern ConVar mani_tk_burn_time;

extern ConVar mani_tk_allow_time_bomb_option;
extern ConVar mani_tk_time_bomb_seconds;
extern ConVar mani_tk_time_bomb_blast_radius;
extern ConVar mani_tk_time_bomb_show_beams;
extern ConVar mani_tk_time_bomb_blast_mode;
extern ConVar mani_tk_time_bomb_beep_radius;

extern ConVar mani_tk_allow_fire_bomb_option;
extern ConVar mani_tk_fire_bomb_seconds;
extern ConVar mani_tk_fire_bomb_blast_radius;
extern ConVar mani_tk_fire_bomb_show_beams;
extern ConVar mani_tk_fire_bomb_blast_mode;
extern ConVar mani_tk_fire_bomb_burn_time;
extern ConVar mani_tk_fire_bomb_beep_radius;

extern ConVar mani_tk_allow_freeze_bomb_option;
extern ConVar mani_tk_freeze_bomb_seconds;
extern ConVar mani_tk_freeze_bomb_blast_radius;
extern ConVar mani_tk_freeze_bomb_show_beams;
extern ConVar mani_tk_freeze_bomb_blast_mode;
extern ConVar mani_tk_freeze_bomb_beep_radius;

extern ConVar mani_tk_allow_beacon_option;
extern ConVar mani_tk_beacon_radius;

extern ConVar mani_tk_allow_bots_to_punish;
extern ConVar mani_tk_allow_bots_to_add_violations;

extern ConVar mani_tk_team_wound_reflect;
extern ConVar mani_tk_team_wound_reflect_threshold;
extern ConVar mani_tk_team_wound_reflect_ratio;
extern ConVar mani_tk_team_wound_reflect_ratio_increase;

extern ConVar mani_adminsay_anonymous;
extern ConVar mani_adminkick_anonymous;
extern ConVar mani_adminslay_anonymous;
extern ConVar mani_adminslap_anonymous;
extern ConVar mani_adminban_anonymous;
extern ConVar mani_adminblind_anonymous;
extern ConVar mani_adminfreeze_anonymous;
extern ConVar mani_admindrug_anonymous;
extern ConVar mani_adminburn_anonymous;
extern ConVar mani_adminteleport_anonymous;
extern ConVar mani_adminvote_anonymous;
extern ConVar mani_adminnoclip_anonymous;
extern ConVar mani_adminmute_anonymous;
extern ConVar mani_admincash_anonymous;
extern ConVar mani_adminsetskin_anonymous;
extern ConVar mani_admindropc4_anonymous;
extern ConVar mani_admintimebomb_anonymous;
extern ConVar mani_adminfirebomb_anonymous;
extern ConVar mani_adminfreezebomb_anonymous;
extern ConVar mani_adminbeacon_anonymous;
extern ConVar mani_adminhealth_anonymous;
extern ConVar mani_admingive_anonymous;
extern ConVar mani_admincolor_anonymous;
extern ConVar mani_admingravity_anonymous;

extern ConVar mani_adminmap_anonymous;
extern ConVar mani_adminswap_anonymous;
extern ConVar mani_admin_burn_time;

extern ConVar mani_adminsay_top_left;
extern ConVar mani_adminsay_chat_area;
extern ConVar mani_adminsay_bottom_area;

extern ConVar mani_stats;
extern ConVar mani_stats_mode;
extern ConVar mani_stats_calculate;
extern ConVar mani_stats_drop_player_days;
extern ConVar mani_stats_kills_required;
extern ConVar mani_stats_top_display_time;
extern ConVar mani_stats_show_rank_to_all;
extern ConVar mani_stats_alternative_rank_message;
extern ConVar mani_stats_write_text_file;
extern ConVar mani_stats_calculate_frequency;
extern ConVar mani_stats_write_frequency_to_disk;
extern ConVar mani_stats_include_bot_kills;
extern ConVar mani_stats_most_destructive;

extern ConVar mani_ff_player_only;
extern ConVar mani_nextmap_player_only;
extern ConVar mani_timeleft_player_only;
extern ConVar mani_thetime_player_only;

extern ConVar mani_war_mode_force_overview_zero;

extern ConVar mani_reserve_slots;
extern ConVar mani_reserve_slots_number_of_slots;
extern ConVar mani_reserve_slots_kick_message;
extern ConVar mani_reserve_slots_redirect_message;
extern ConVar mani_reserve_slots_redirect;
extern ConVar mani_reserve_slots_allow_slot_fill;
extern ConVar mani_reserve_slots_kick_method;
extern ConVar mani_reserve_slots_include_admin;
extern ConVar mani_reserve_slots_ip_keep_history;
extern ConVar mani_reverse_admin_flags;
extern ConVar mani_reverse_immunity_flags;
extern ConVar mani_reserve_slots_enforce_password;

extern ConVar mani_military_time;

extern ConVar mani_chat_flood_time;
extern ConVar mani_chat_flood_message;
extern ConVar mani_use_ma_in_say_command;

extern ConVar mani_sounds_per_round;
extern ConVar mani_sounds_filter_if_dead;
extern ConVar mani_show_victim_stats;
extern ConVar mani_show_victim_stats_inflicted_only;
extern ConVar mani_play_sound_type;

extern ConVar mani_use_amx_style_menu;
extern ConVar mani_autobalance_teams;
extern ConVar mani_autobalance_mode;
extern ConVar mani_thetime_timezone;

extern ConVar mani_voting;
extern ConVar mani_vote_allow_user_vote_map;
extern ConVar mani_vote_allow_user_vote_map_extend;
extern ConVar mani_vote_allow_user_vote_kick;
extern ConVar mani_vote_allow_user_vote_kick_ghost;
extern ConVar mani_vote_allow_user_vote_ban;
extern ConVar mani_vote_allow_user_vote_ban_ghost;
extern ConVar mani_vote_allow_end_of_map_vote;

extern ConVar mani_vote_dont_show_if_alive;
extern ConVar mani_vote_dont_show_last_maps;
extern ConVar mani_vote_extend_time;
extern ConVar mani_vote_extend_rounds;
extern ConVar mani_vote_allow_extend;
extern ConVar mani_vote_max_extends;
extern ConVar mani_vote_allowed_voting_time;
extern ConVar mani_vote_mapcycle_mode_for_random_map_vote;
extern ConVar mani_vote_mapcycle_mode_for_admin_map_vote;
extern ConVar mani_vote_time_before_end_of_map_vote;
extern ConVar mani_vote_rounds_before_end_of_map_vote;
extern ConVar mani_vote_max_maps_for_end_of_map_vote;
extern ConVar mani_vote_end_of_map_percent_required;
extern ConVar mani_vote_rcon_percent_required;
extern ConVar mani_vote_question_percent_required;
extern ConVar mani_vote_map_percent_required;
extern ConVar mani_vote_random_map_percent_required;
extern ConVar mani_vote_extend_percent_required;
extern ConVar mani_vote_show_vote_mode;
extern ConVar mani_vote_end_of_map_swap_team;
extern ConVar mani_vote_randomize_extend_vote;

extern ConVar mani_vote_user_vote_map_percentage;
extern ConVar mani_vote_user_vote_map_time_before_vote;
extern ConVar mani_vote_user_vote_map_minimum_votes;

extern ConVar mani_vote_user_vote_kick_mode;
extern ConVar mani_vote_user_vote_kick_percentage;
extern ConVar mani_vote_user_vote_kick_time_before_vote;
extern ConVar mani_vote_user_vote_kick_minimum_votes;

extern ConVar mani_vote_user_vote_ban_mode;
extern ConVar mani_vote_user_vote_ban_percentage;
extern ConVar mani_vote_user_vote_ban_time_before_vote;
extern ConVar mani_vote_user_vote_ban_minimum_votes;
extern ConVar mani_vote_user_vote_ban_time;
extern ConVar mani_vote_user_vote_ban_type;


extern ConVar mani_vote_allow_rock_the_vote;
extern ConVar mani_vote_rock_the_vote_threshold_percent;
extern ConVar mani_vote_rock_the_vote_threshold_minimum;
extern ConVar mani_vote_rock_the_vote_percent_required;
extern ConVar mani_vote_time_before_rock_the_vote;
extern ConVar mani_vote_rock_the_vote_number_of_nominations;
extern ConVar mani_vote_rock_the_vote_number_of_maps;

extern ConVar mani_player_name_change_threshold;
extern ConVar mani_player_name_change_reset;
extern ConVar mani_player_name_change_punishment;
extern ConVar mani_player_name_change_ban_time;

//extern ConVar mani_player_cheat_punishment;
//extern ConVar mani_player_cheat_ban_time;

extern ConVar mani_allow_chat_to_admin;

extern ConVar mani_filter_words_mode;
extern ConVar mani_filter_words_warning;

extern ConVar mani_adjust_time;

extern ConVar mani_player_settings_quake;
extern ConVar mani_player_settings_sounds;
extern ConVar mani_player_settings_damage;
extern ConVar mani_player_settings_death_beam;
extern ConVar mani_player_settings_destructive;
extern ConVar mani_player_settings_vote_progress;

extern ConVar mani_mute_con_command_spam;

extern ConVar mani_skins_admin;
extern ConVar mani_skins_reserved;
extern ConVar mani_skins_public;
extern ConVar mani_skins_force_public;
extern ConVar mani_skins_setskin_misc_only;
extern ConVar mani_skins_force_choose_on_join;
extern ConVar mani_skins_random_bot_skins;

extern ConVar mani_show_death_beams;
extern ConVar mani_sort_menus;

extern ConVar mani_blind_ghosters;

extern ConVar mani_map_adverts;
extern ConVar mani_map_adverts_in_war;

extern ConVar mani_spawnpoints_mode;

extern ConVar mani_spray_tag;
extern ConVar mani_spray_tag_block_mode;
extern ConVar mani_spray_tag_block_message;
extern ConVar mani_spray_tag_spray_duration;
extern ConVar mani_spray_tag_spray_distance_limit;
extern ConVar mani_spray_tag_spray_highlight;
extern ConVar mani_spray_tag_ban_time;
extern ConVar mani_spray_tag_warning_message;
extern ConVar mani_spray_tag_kick_message;
extern ConVar mani_spray_tag_ban_message;
extern ConVar mani_spray_tag_perm_ban_message;
extern ConVar mani_spray_tag_slap_damage;

extern ConVar mani_external_stats_log;
extern ConVar mani_external_stats_log_allow_war_logs;

extern ConVar mani_hostage_follow_warning;

extern ConVar mani_afk_kicker;

extern ConVar mani_all_see_ma_rates;

extern ConVar mani_admin_temp_ban_time_limit;

extern ConVar mani_admin_temp_mute_time_limit;
extern ConVar mani_admin_default_ban_reason;
extern ConVar mani_admin_default_mute_reason;

extern ConVar mani_bans_max_shown_in_menu;
#endif
