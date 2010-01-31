/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef COMMAND_H
#define COMMAND_H

#include "../common/seperator.h"
#include "../common/EQStream.h"
#include "client.h"

#define	COMMAND_CHAR	'#'
#define CMDALIASES	5

typedef void (*CmdFuncPtr)(Client *,const Seperator *);

// this is a command list item
/*struct cl_struct
{
  char *command[CMDALIASES];			// the command(s)
  char *desc;					// description of command
  CmdFuncPtr function;				// the function to call
  int access;					// the required 'status' level
  
  struct cl_struct *next;			// linked list
};

extern struct cl_struct *commandlist;		// the head of the list
*/

typedef struct {
	const char *command[CMDALIASES];			// the command(s)
	int access;
	const char *desc;		// description of command
	CmdFuncPtr function;	//null means perl function
} CommandRecord;

extern int (*command_dispatch)(Client *,char const*);
extern int commandcount;			// number of commands loaded

// the command system:
int command_init(void);
void command_deinit(void);
int command_add(const char *command_string, const char *desc, int access, CmdFuncPtr function);
int command_notavail(Client *c, const char *message);
int command_realdispatch(Client *c, char const *message);
void command_logcommand(Client *c, const char *message);
int command_add_perl(const char *command_string, const char *desc, int access);
void command_clear_perl();

//commands
void command_resetaa(Client* c,const Seperator *sep);
void command_bind(Client* c,const Seperator *sep);
void command_sendop(Client *c, const Seperator *sep);
void command_optest(Client *c, const Seperator *sep);
void command_setstat(Client *c, const Seperator *sep);
void command_incstat(Client *c, const Seperator *sep);
void command_help(Client *c, const Seperator *sep);
void command_version(Client *c, const Seperator *sep);
void command_eitem(Client *c, const Seperator *sep);
void command_setfaction(Client *c, const Seperator *sep);
void command_serversidename(Client *c, const Seperator *sep);
void command_testspawnkill(Client *c, const Seperator *sep);
void command_testspawn(Client *c, const Seperator *sep);
void command_wc(Client *c, const Seperator *sep);
void command_numauths(Client *c, const Seperator *sep);
void command_setanim(Client *c, const Seperator *sep);
void command_connectworldserver(Client *c, const Seperator *sep);
void command_serverinfo(Client *c, const Seperator *sep);
void command_crashtest(Client *c, const Seperator *sep);
void command_getvariable(Client *c, const Seperator *sep);
void command_chat(Client *c, const Seperator *sep);
void command_showpetspell(Client *c, const Seperator *sep);
void command_ipc(Client *c, const Seperator *sep);
void command_npcloot(Client *c, const Seperator *sep);
void command_log(Client *c, const Seperator *sep);
void command_gm(Client *c, const Seperator *sep);
void command_summon(Client *c, const Seperator *sep);
void command_zone(Client *c, const Seperator *sep);
void command_zone_instance(Client *c, const Seperator *sep);
void command_peqzone(Client *c, const Seperator *sep);
void command_showbuffs(Client *c, const Seperator *sep);
void command_movechar(Client *c, const Seperator *sep);
void command_viewpetition(Client *c, const Seperator *sep);
void command_petitioninfo(Client *c, const Seperator *sep);
void command_delpetition(Client *c, const Seperator *sep);
void command_listnpcs(Client *c, const Seperator *sep);
void command_date(Client *c, const Seperator *sep);
void command_timezone(Client *c, const Seperator *sep);
void command_synctod(Client *c, const Seperator *sep);
void command_invul(Client *c, const Seperator *sep);
void command_hideme(Client *c, const Seperator *sep);
void command_emote(Client *c, const Seperator *sep);
void command_fov(Client *c, const Seperator *sep);
void command_manastat(Client *c, const Seperator *sep);
void command_npcstats(Client *c, const Seperator *sep);
void command_zclip(Client *c, const Seperator *sep);
void command_npccast(Client *c, const Seperator *sep);
void command_zstats(Client *c, const Seperator *sep);
void command_permaclass(Client *c, const Seperator *sep);
void command_permarace(Client *c, const Seperator *sep);
void command_permagender(Client *c, const Seperator *sep);
void command_weather(Client *c, const Seperator *sep);
void command_zheader(Client *c, const Seperator *sep);
void command_zsky(Client *c, const Seperator *sep);
void command_zcolor(Client *c, const Seperator *sep);
void command_spon(Client *c, const Seperator *sep);
void command_spoff(Client *c, const Seperator *sep);
void command_itemtest(Client *c, const Seperator *sep);
void command_gassign(Client *c, const Seperator *sep);
void command_setitemstatus(Client *c, const Seperator *sep);
void command_ai(Client *c, const Seperator *sep);
void command_worldshutdown(Client *c, const Seperator *sep);
void command_sendzonespawns(Client *c, const Seperator *sep);
void command_zsave(Client *c, const Seperator *sep);
void command_dbspawn2(Client *c, const Seperator *sep);
void command_copychar(Client *c, const Seperator *sep);
void command_shutdown(Client *c, const Seperator *sep);
void command_delacct(Client *c, const Seperator *sep);
void command_setpass(Client *c, const Seperator *sep);
void command_grid(Client *c, const Seperator *sep);
void command_wp(Client *c, const Seperator *sep);
void command_wp(Client *c, const Seperator *sep);
void command_iplookup(Client *c, const Seperator *sep);
void command_size(Client *c, const Seperator *sep);
void command_mana(Client *c, const Seperator *sep);
void command_flymode(Client *c, const Seperator *sep);
void command_showskills(Client *c, const Seperator *sep);
void command_findspell(Client *c, const Seperator *sep);
void command_castspell(Client *c, const Seperator *sep);
void command_setlanguage(Client *c, const Seperator *sep);
void command_setskill(Client *c, const Seperator *sep);
void command_setskillall(Client *c, const Seperator *sep);
void command_race(Client *c, const Seperator *sep);
void command_gender(Client *c, const Seperator *sep);
void command_makepet(Client *c, const Seperator *sep);
void command_level(Client *c, const Seperator *sep);
void command_spawn(Client *c, const Seperator *sep);
void command_texture(Client *c, const Seperator *sep);
void command_npctypespawn(Client *c, const Seperator *sep);
void command_heal(Client *c, const Seperator *sep);
void command_appearance(Client *c, const Seperator *sep);
void command_charbackup(Client *c, const Seperator *sep);
void command_nukeitem(Client *c, const Seperator *sep);
void command_peekinv(Client *c, const Seperator *sep);
void command_findnpctype(Client *c, const Seperator *sep);
void command_findzone(Client *c, const Seperator *sep);
void command_viewnpctype(Client *c, const Seperator *sep);
void command_reloadqst(Client *c, const Seperator *sep);
void command_reloadzps(Client *c, const Seperator *sep);
void command_zoneshutdown(Client *c, const Seperator *sep);
void command_zonebootup(Client *c, const Seperator *sep);
void command_kick(Client *c, const Seperator *sep);
void command_attack(Client *c, const Seperator *sep);
void command_lock(Client *c, const Seperator *sep);
void command_unlock(Client *c, const Seperator *sep);
void command_motd(Client *c, const Seperator *sep);
void command_listpetition(Client *c, const Seperator *sep);
void command_equipitem(Client *c, const Seperator *sep);
void command_zonelock(Client *c, const Seperator *sep);
void command_corpse(Client *c, const Seperator *sep);
void command_fixmob(Client *c, const Seperator *sep);
void command_gmspeed(Client *c, const Seperator *sep);
void command_title(Client *c, const Seperator *sep);
void command_titlesuffix(Client *c, const Seperator *sep);
void command_spellinfo(Client *c, const Seperator *sep);
void command_lastname(Client *c, const Seperator *sep);
void command_memspell(Client *c, const Seperator *sep);
void command_save(Client *c, const Seperator *sep);
void command_showstats(Client *c, const Seperator *sep);
void command_mystats(Client *c, const Seperator *sep);
void command_myskills(Client *c, const Seperator *sep);
void command_depop(Client *c, const Seperator *sep);
void command_depopzone(Client *c, const Seperator *sep);
void command_repop(Client *c, const Seperator *sep);
void command_spawnstatus(Client *c, const Seperator *sep);
void command_nukebuffs(Client *c, const Seperator *sep);
void command_zuwcoords(Client *c, const Seperator *sep);
void command_zunderworld(Client *c, const Seperator *sep);
void command_zsafecoords(Client *c, const Seperator *sep);
void command_freeze(Client *c, const Seperator *sep);
void command_unfreeze(Client *c, const Seperator *sep);
void command_pvp(Client *c, const Seperator *sep);
void command_setxp(Client *c, const Seperator *sep);
void command_setpvppoints(Client *c, const Seperator *sep);
void command_name(Client *c, const Seperator *sep);
void command_npcspecialattk(Client *c, const Seperator *sep);
void command_kill(Client *c, const Seperator *sep);
void command_haste(Client *c, const Seperator *sep);
void command_damage(Client *c, const Seperator *sep);
void command_zonespawn(Client *c, const Seperator *sep);
void command_npcspawn(Client *c, const Seperator *sep);
void command_spawnfix(Client *c, const Seperator *sep);
void command_loc(Client *c, const Seperator *sep);
void command_goto(Client *c, const Seperator *sep);
#ifdef BUGTRACK
void command_bug(Client *c, const Seperator *sep);
#endif
void command_iteminfo(Client *c, const Seperator *sep);
void command_uptime(Client *c, const Seperator *sep);
void command_flag(Client *c, const Seperator *sep);
void command_time(Client *c, const Seperator *sep);
void command_guild(Client *c, const Seperator *sep);
bool helper_guild_edit(Client *c, int32 dbid, int32 eqid, int8 rank, const char* what, const char* value);
void command_zonestatus(Client *c, const Seperator *sep);
void command_manaburn(Client *c, const Seperator *sep);
void command_viewmessage(Client *c, const Seperator *sep);
void command_doanim(Client *c, const Seperator *sep);
void command_randomfeatures(Client *c, const Seperator *sep);
void command_face(Client *c, const Seperator *sep);
void command_helm(Client *c, const Seperator *sep);
void command_hair(Client *c, const Seperator *sep);
void command_haircolor(Client *c, const Seperator *sep);
void command_beard(Client *c, const Seperator *sep);
void command_beardcolor(Client *c, const Seperator *sep);
void command_tattoo(Client *c, const Seperator *sep);
void command_heritage(Client *c, const Seperator *sep);
void command_details(Client *c, const Seperator *sep);
void command_scribespells(Client *c, const Seperator *sep);
void command_unscribespells(Client *c, const Seperator *sep);
void command_wpinfo(Client *c, const Seperator *sep);
void command_wpadd(Client *c, const Seperator *sep);
void command_interrupt(Client *c, const Seperator *sep);
void command_d1(Client *c, const Seperator *sep);
void command_summonitem(Client *c, const Seperator *sep);
void command_giveitem(Client *c, const Seperator *sep);
void command_givemoney(Client *c, const Seperator *sep);
void command_itemsearch(Client *c, const Seperator *sep);
void command_datarate(Client *c, const Seperator *sep);
void command_setaaxp(Client *c, const Seperator *sep);
void command_setaapts(Client *c, const Seperator *sep);
void command_setcrystals(Client *c, const Seperator *sep);
void command_stun(Client *c, const Seperator *sep);
void command_ban(Client *c, const Seperator *sep);
void command_suspend(Client *c, const Seperator *sep);
void command_ipban(Client *c, const Seperator *sep);
void command_oocmute(Client *c, const Seperator *sep);
void command_revoke(Client *c, const Seperator *sep);
void command_checklos(Client *c, const Seperator *sep);
void command_set_adventure_points(Client *c, const Seperator *sep);
void command_npcsay(Client *c, const Seperator *sep);
void command_npcshout(Client *c, const Seperator *sep);
void command_npcemote(Client *c, const Seperator *sep);
void command_npcedit(Client *c, const Seperator *sep);
void command_timers(Client *c, const Seperator *sep);
void command_undye(Client *c, const Seperator *sep);
void command_undyeme(Client *c, const Seperator *sep);
void command_hp(Client *c, const Seperator *sep);
void command_ginfo(Client *c, const Seperator *sep);
void command_logs(Client *c, const Seperator *sep);
void command_nologs(Client *c, const Seperator *sep);
void command_logsql(Client *c, const Seperator *sep);
void command_qglobal(Client *c, const Seperator *sep);
void command_fear(Client *c, const Seperator *sep);
void command_path(Client *c, const Seperator *sep);
void command_ginfo(Client *c, const Seperator *sep);
void command_opcode(Client *c, const Seperator *sep);
void command_aggro(Client *c, const Seperator *sep);
void command_hatelist(Client *c, const Seperator *sep);
void command_aggrozone(Client *c, const Seperator *sep);
void command_reloadstatic(Client *c, const Seperator *sep);
void command_flags(Client *c, const Seperator *sep);
void command_flagedit(Client *c, const Seperator *sep);
void command_mlog(Client *c, const Seperator *sep);
void command_serverrules(Client *c, const Seperator *sep);
void command_acceptrules(Client *c, const Seperator *sep);
void command_guildcreate(Client *c, const Seperator *sep);
void command_guildapprove(Client *c, const Seperator *sep);
void command_guildlist(Client *c, const Seperator *sep);
void command_rules(Client *c, const Seperator *sep);
void command_task(Client *c, const Seperator *sep);
void command_reloadtitles(Client *c, const Seperator *sep);
void command_altactivate(Client *c, const Seperator *sep);
void command_refundaa(Client *c, const Seperator *sep);
void command_melody(Client *c, const Seperator *sep);
void command_traindisc(Client *c, const Seperator *sep);
void command_deletegraveyard(Client *c, const Seperator *sep);
void command_setgraveyard(Client *c, const Seperator *sep);
void command_getplayerburriedcorpsecount(Client *c, const Seperator *sep);
void command_summonburriedplayercorpse(Client *c, const Seperator *sep);
void command_unscribespell(Client *c, const Seperator *sep);
void command_scribespell(Client *c, const Seperator *sep);
void command_refreshgroup(Client *c, const Seperator *sep);
void command_advnpcspawn(Client *c, const Seperator *sep);
void command_modifynpcstat(Client *c, const Seperator *sep);
void command_instance(Client *c, const Seperator *sep);
void command_setstartzone(Client *c, const Seperator *sep);
void command_netstats(Client *c, const Seperator *sep);
void command_object(Client* c, const Seperator *sep);
void command_raidloot(Client* c, const Seperator *sep);
void command_globalview(Client* c, const Seperator *sep);
void command_distance(Client *c, const Seperator *sep);

#ifdef EMBPERL
void command_embperl_plugin(Client *c, const Seperator *sep);
void command_embperl_eval(Client *c, const Seperator *sep);
void command_reloadpl(Client *c, const Seperator *sep);
#endif

#ifdef EQPROFILE
void command_profiledump(Client *c, const Seperator *sep);
void command_profilereset(Client *c, const Seperator *sep);
#endif

#ifdef PACKET_PROFILER
void command_packetprofile(Client *c, const Seperator *sep);
#endif

#ifdef BOTS
#include "bot.h"
void command_bot(Client*c, const Seperator *sep);
#endif

#endif

