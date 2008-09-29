/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

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
#include "../common/debug.h"
#include "masterentity.h"
#include "worldserver.h"
#include "zonedb.h"
#include "spdat.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "petitions.h"
#include "../common/serverinfo.h"
#include "../common/ZoneNumbers.h"
#include "../common/moremath.h"
#include "../common/guilds.h"
#include "StringIDs.h"
#include "NpcAI.h"

float Client::GetActSpellRange(int16 spell_id, float range)
{
//	int16 modspellid = 0;
	float extrange = 100;

	/*if (GetExtendedRangeItem(modspellid, NULL)) {
		extrange = GenericFocus(spell_id, modspellid);
	}*/
	extrange += GetFocusEffect(focusRange, spell_id);
	
	return (range * extrange) / 100;
}

sint32 Client::GetActSpellDamage(int16 spell_id, sint32 value) {
	// Important variables:
	// value: the actual damage after resists, passed from Mob::SpellEffect
	// modifier: modifier to damage (from spells & focus effects?)
	// ratio: % of the modifier to apply (from AAs & natural bonus?)
	// chance: critital chance %
	
//	int16 modspellid = 0;
	
	//all of the ordering and stacking in here might be wrong, but I dont care right now.
	
	
	sint32 modifier = 100;
	
	//Dunno if this makes sense:
	if (spells[spell_id].resisttype > 0)
		modifier += GetFocusEffect((focusType)(0-spells[spell_id].resisttype), spell_id);
	
	
	int tt = spells[spell_id].targettype;
	if (tt == ST_UndeadAE || tt == ST_Undead || tt == ST_Summoned) {
		//undead/summoned spells
		modifier += GetFocusEffect(focusImprovedUndeadDamage, spell_id);
    } else {
    	//damage spells.
		modifier += GetFocusEffect(focusImprovedDamage, spell_id);
		//if (GetImprovedDamageItem(modspellid, NULL))
		//	modifier = GenericFocus(spell_id, modspellid);
	}
	
	//these spell IDs could be wrong
	if (spell_id == SPELL_LEECH_TOUCH) {	//leech touch
		value -= GetAA(aaConsumptionoftheSoul) * 500;
	}
	if (spell_id == SPELL_IMP_HARM_TOUCH) {	//harm touch
		switch(GetAA(aaUnholyTouch)) {
			case 1:
				modifier += 25;
				break;
			case 2:
				modifier += 50;
				break;
			case 3:
				modifier += 75;
				break;
		}
	}
	
	//spell crits, dont make sense if cast on self.
	if(tt != ST_Self) {
		int chance = 0;
		sint32 ratio = 0;

		//normal spell crit
		if(GetClass() == WIZARD && GetLevel() > 11) {
			chance += 7;
			ratio += 15;
		}
		
		//Normal EQ: no class that has ingenuity has reg spell crit AAs too but people
		//are free to customize so lets make sure they don't stack oddly.
		//afaik all ranks provide a 100% bonus in damage on critical
		switch(GetAA(aaIngenuity))
		{
		case 1:
		case 2:
		case 3: 
			if(ratio < 100)
				ratio = 100;
			break;
		default:
			break;
		}
		
		if(tt == ST_Target || tt == ST_Summoned || tt == ST_Undead) {
			//DD spells only...
			//reference: http://www.graffe.com/AA/
			switch (GetAA(aaSpellCastingFury)) //not sure why this was different from Mastery before, both are DD only
			{
				case 1:
					chance += 2;
					ratio += 33;
					break;
				case 2:
					chance += 4; //some reports between 4.5% & 5%, AA description indicates 4%
					ratio += 66;
					break;
				case 3:
					chance += 7;
					ratio += 100;
					break;
			}
			switch (GetAA(aaSpellCastingFuryMastery)) //ratio should carry over from Spell Casting Fury, which is 100% for all ranks
			{
				case 1:
					chance += 3; //10%, Graffe = 9%?
					break;
				case 2:
					chance += 5; //12%, Graffe = 11%?
					break;
				case 3:
					chance += 7; //14%, Graffe = 13%?
					break;
			}
			chance += GetAA(aaFuryofMagic) * 2;  //doesn't look like this is used
			chance += GetAA(aaFuryofMagicMastery) * 2; //doesn't look like this is used
			chance += GetAA(aaFuryofMagicMastery2) * 2;	//this is the current one used in DB; 16%, 18%, 20%; Graffe guesses 18-19% max
			chance += GetAA(aaAdvancedFuryofMagicMastery) * 2; //guessing, not much data on it

			
			if(ratio < 100)	//chance increase and ratio are made up, not confirmed
				ratio = 100;
		} else if(tt == ST_Tap) {
			if(ratio < 100)	//chance increase and ratio are made up, not confirmed
				ratio = 100;
			
			if(spells[spell_id].classes[SHADOWKNIGHT-1] >= 254 && spell_id != SPELL_LEECH_TOUCH){
				switch (GetAA(aaSoulAbrasion)) //Soul Abrasion
				{
					case 1:
						modifier += 100;
						break;
					case 2:
						modifier += 200;
						break;
					case 3:
						modifier += 300;
						break;
				}
			}
		}
		
		chance += GetAA(aaIngenuity); //nothing stating it's DD only, so we'll apply to all damage spells
		
		chance += GetFocusEffect(focusImprovedCritical, spell_id);

		//crit damage modifiers
		if (GetClass() == WIZARD) { //wizards get an additional bonus
			ratio += GetAA(aaDestructiveFury) * 8; //108%, 116%, 124%, close to Graffe's 207%, 215%, & 225%
		} else {
			switch (GetAA(aaDestructiveFury)) //not quite linear
			{
				case 1:
					ratio += 4; //104%, Graffe = 103%
					break;
				case 2:
					ratio += 8; //108%, Graffe = 107%
					break;
				case 3:
					ratio += 16; //116%, Graffe = 115%
			}
		}
		
		if(chance > 0 && MakeRandomInt(0,100) <= chance) {
			modifier += modifier*ratio/100;
			entity_list.MessageClose(this, false, 100, MT_SpellCrits, "%s delivers a critical blast! (%d)", GetName(), ((-value * modifier) / 100));	
		}
	}
	
	return (value * modifier) / 100;
}

sint32 Client::GetActSpellHealing(int16 spell_id, sint32 value) {
//	int16 modspellid = 0;

	sint32 modifier = 100;
	
	modifier += GetFocusEffect(focusImprovedHeal, spell_id);
	//if (GetImprovedHealingItem(modspellid, NULL))
	//	modifier = GenericFocus(spell_id, modspellid);
						
	if(spells[spell_id].buffduration < 1) {
		//non-dot
		switch(GetAA(aaHealingAdept)) {
		case 1:
			modifier += 2;
			break;
		case 2:
			modifier += 5;
			break;
		case 3:
			modifier += 10;
			break;
		}
		
		switch(GetAA(aaAdvancedHealingAdept)) {
		case 1:
			modifier += 3;
			break;
		case 2:
			modifier += 6;
			break;
		case 3:
			modifier += 9;
			break;
		}
		
		int chance = 0;
		switch(GetAA(aaHealingGift)) {
		case 1:
			chance = 3;
			break;
		case 2:
			chance = 6;
			break;
		case 3:
			chance = 10;
			break;
		}
		chance += GetAA(aaAdvancedHealingGift) * 2;

		if(spells[spell_id].targettype == ST_Tap)
		{
			chance += GetAA(aaTheftofLife) * 3;
		}
		
		if(MakeRandomInt(0,100) < chance) {
			entity_list.MessageClose(this, false, 100, MT_SpellCrits, "%s performs an exceptional heal! (%d)", GetName(), ((value * modifier) / 50));		
			return (value * modifier) / 50;
		}
		else{
			return (value * modifier) / 100;
		}		
	}
					
	return (value * modifier) / 100;
}

/*sint32 Client::GetDotFocus(int16 spell_id, sint32 value) {
//	int16 modspellid = 0;

	int modifier = 100;
	

	modifier += GetFocusEffect(focusImprovedDOT, spell_id);
	//if (GetImprovedDamageItem(modspellid, NULL)) {
	//		modifier = GenericFocus(spell_id, modspellid);
	//}
	
	//why is this ammount random?
	int randamount = MakeRandomInt(1, (100-modifier));
	return (value * (100-randamount)) / 100;
}*/

sint32 Client::GetActSpellCost(int16 spell_id, sint32 cost)
{
	sint32 Result = 0;

	if(GetClass() == WIZARD || GetClass() == ENCHANTER || GetClass() == MAGICIAN || GetClass() == NECROMANCER || GetClass() == DRUID || GetClass() == SHAMAN || GetClass() == CLERIC || GetClass() == BARD) {
		// This formula was derived from the following resource:
		// http://www.eqsummoners.com/eq1/specialization-library.html
		// WildcardX
		float PercentManaReduction = 0;
		float PercentOfMaxSpecializeSkill = 0;
		float MaxSpecilizationSkillAllowed = MaxSkill(spells[spell_id].skill);
		float SpecializeSkill = GetSpecializeSkillValue(spell_id);
		int SuccessChance = MakeRandomInt(0, 100);
		
		if(MaxSpecilizationSkillAllowed > 0)
			PercentOfMaxSpecializeSkill = SpecializeSkill / MaxSpecilizationSkillAllowed;
		
		if(SuccessChance <= (PercentOfMaxSpecializeSkill * 100))
			PercentManaReduction = (SpecializeSkill * .053) - 5.65;
		
		PercentManaReduction += GetAA(aaSpellCastingMastery);
		PercentManaReduction += GetAA(aaAdvancedSpellCastingMastery);
		PercentManaReduction += this->CastToClient()->GetFocusEffect(focusManaCost, spell_id);
		cost -= (cost * (PercentManaReduction / 100));
	}

	if(cost < 0)
		cost = 0;

	Result = cost;

	return Result;

	/*
	//	int16 modspellid = 0;

	int reduce = 100;
	
	reduce -= GetFocusEffect(focusManaCost, spell_id);
	//if (GetReduceManaCostItem(modspellid, NULL)) {
	//	reduce = GenericFocus(spell_id, modspellid);
	//}

	//reduce -= CastToClient()->GetFocusEffect(9, spell_id);
	
	uint32 spec_value = GetSpecializeSkillValue(spell_id);
		//VERY rough success formula, needs research
	uint32 spec_goal = spec_value + 98 + 5 * (GetAA(aaSpellCastingMastery) + GetAA(aaAdvancedSpellCastingMastery));
	if(spec_value > 0 && (spec_goal < (uint32)MakeRandomInt(0, 600))) {
		reduce -= spec_value * SPECIALIZE_MANA_REDUCE / 200;
	}
	//arbitrary rule: cannot reduce it below 10%
	if(reduce < 10)
		reduce = 10;
	return (cost * reduce) / 100;
	*/
}

sint32 Client::GetActSpellDuration(int16 spell_id, sint32 duration)
{
//	int16 modspellid = 0;

	int increase = 100;
	increase += GetFocusEffect(focusSpellDuration, spell_id);
//	if (GetIncreaseSpellDurationItem(modspellid, NULL)) {
//		increase = GenericFocus(spell_id, modspellid);
//	}
	
	switch(GetAA(aaSpellCastingReinforcement)) {
	case 1:
		increase += 5;
		break;
	case 2:
		increase += 15;
		break;
	case 3:
		increase += 30;
		if (GetAA(aaSpellCastingReinforcementMastery) == 1)
			increase += 20;
		break;
	}
	
	return (duration * increase) / 100;
}

sint32 Client::GetActSpellCasttime(int16 spell_id, sint32 casttime)
{
	
	sint32 cast_reducer = 0;
	cast_reducer += GetFocusEffect(focusSpellHaste, spell_id);

//	int16 modspellid = 0;
//	if (GetReduceCastTimeItem(modspellid, NULL)) {
//		reduce += GenericFocus(spell_id, modspellid);
//	}

	//this function loops through the effects of spell_id many times
	//could easily be consolidated.

	if (GetLevel() >= 51 && casttime >= 3000 && !BeneficialSpell(spell_id) 
		&& (GetClass() == SHADOWKNIGHT || GetClass() == RANGER 
			|| GetClass() == PALADIN || GetClass() == BEASTLORD ))
		cast_reducer += (GetLevel()-50)*3;
	
	if(casttime >= 4000 && BeneficialSpell(spell_id) && CalcBuffDuration(this, this, spell_id) > 0){
		switch (GetAA(aaSpellCastingDeftness)) {
			case 1:
				cast_reducer += 5;
				break;
			case 2:
				cast_reducer += 10;
				break;
			case 3:
				cast_reducer += 25;
				break;
		}

		switch (GetAA(aaQuickBuff))
		{
			case 1:
				cast_reducer += 10;
				break;
			case 2:
				cast_reducer += 25;
				break;
			case 3:
				cast_reducer += 50;
				break;
		}
	}
	if (IsSummonSpell(spell_id))
	{
		switch (GetAA(aaQuickSummoning))
		{
			case 1:
				cast_reducer += 10;
				break;
			case 2:
				cast_reducer += 25;
				break;
			case 3:
				cast_reducer += 50;
				break;
		}
	}
	if (IsEvacSpell(spell_id))
	{
		switch (GetAA(aaQuickEvacuation))
		{
			case 1:
				cast_reducer += 10;
				break;
			case 2:
				cast_reducer += 25;
				break;
			case 3:
				cast_reducer += 50;
				break;
		}
	}
	if (IsDamageSpell(spell_id) && spells[spell_id].cast_time >= 4000)
	{
		switch (GetAA(aaQuickDamage))
		{
			case 1:
				cast_reducer += 2;
				break;
			case 2:
				cast_reducer += 5;
				break;
			case 3:
				cast_reducer += 10;
				break;
		}
	}
	if (cast_reducer > 50)
		cast_reducer = 50;	//is this just an arbitrary limit?
	
	casttime = (casttime*(100 - cast_reducer)/100);
	
	return casttime;
}

bool Client::TrainDiscipline(int32 itemid) {
	
	//get the item info
	const Item_Struct *item = database.GetItem(itemid);
	if(item == NULL) {
		Message(13, "Unable to find the tome you turned in!");
		LogFile->write(EQEMuLog::Error, "Unable to find turned in tome id %lu\n", itemid);
		return(false);
	}
	
	if(item->ItemClass != ItemClassCommon || item->ItemType != ItemTypeSpell) {
		Message(13, "Invalid item type, you cannot learn from this item.");
		//summon them the item back...
		SummonItem(itemid);
		return(false);
	}
	
	//Need a way to determine the difference between a spell and a tome
	//so they cant turn in a spell and get it as a discipline
	//this is kinda a hack:
	if(!(
		item->Name[0] == 'T' &&
		item->Name[1] == 'o' &&
		item->Name[2] == 'm' &&
		item->Name[3] == 'e' &&
		item->Name[4] == ' '
		)) {
		Message(13, "This item is not a tome.");
		//summon them the item back...
		SummonItem(itemid);
		return(false);
	}
	
	int myclass = GetClass();
	if(myclass == WIZARD || myclass == ENCHANTER || myclass == MAGICIAN || myclass == NECROMANCER) {
		Message(13, "Your class cannot learn from this tome.");
		//summon them the item back...
		SummonItem(itemid);
		return(false);
	}
	
	//make sure we can train this...
	//can we use the item?
	uint32 cbit = 1 << (myclass-1);
	if(!(item->Classes & cbit)) {
		Message(13, "Your class cannot learn from this tome.");
		//summon them the item back...
		SummonItem(itemid);
		return(false);
	}
	
	int32 spell_id = item->Scroll.Effect;
	if(!IsValidSpell(spell_id)) {
		Message(13, "This tome contains invalid knowledge.");
		return(false);
	}
	
	//can we use the spell?
	const SPDat_Spell_Struct &spell = spells[spell_id];
	int8 level_to_use = spell.classes[myclass - 1];
	if(level_to_use == 255) {
		Message(13, "Your class cannot learn from this tome.");
		//summon them the item back...
		SummonItem(itemid);
		return(false);
	}
	
	if(level_to_use > GetLevel()) {
//		Message_StringID(13, DISC_LEVEL_ERROR, level_to_use);
		Message(13, "You must be at least level %d to learn this discipline.", level_to_use);
		//summon them the item back...
		SummonItem(itemid);
		return(false);
	}
	
	//add it to PP.
	int r;
	for(r = 0; r < MAX_PP_DISCIPLINES; r++) {
		if(m_pp.disciplines.values[r] == spell_id) {
			Message(13, "You allready know this discipline.");
			//summon them the item back...
			SummonItem(itemid);
			return(false);
		} else if(m_pp.disciplines.values[r] == 0) {
			m_pp.disciplines.values[r] = spell_id;
			SendDisciplineUpdate();
			Message(0, "You have learned a new discipline!");
			return(true);
		}
	}
	Message(13, "You have learned too many disciplines and can learn no more.");
	return(false);
}

void Client::SendDisciplineUpdate() {
	//this dosent seem to work right now
	
	EQApplicationPacket app(OP_DisciplineUpdate, sizeof(Disciplines_Struct));
	Disciplines_Struct *d = (Disciplines_Struct*)app.pBuffer;
	//dunno why I dont just send the one from m_pp
	memcpy(d, &m_pp.disciplines, sizeof(m_pp.disciplines));

	QueuePacket(&app);
}

bool Client::UseDiscipline(int32 spell_id, int32 target) {
	//make sure we have the spell...
	int r;
	for(r = 0; r < MAX_PP_DISCIPLINES; r++) {
		if(m_pp.disciplines.values[r] == spell_id)
			break;
	}
	if(r == MAX_PP_DISCIPLINES)
		return(false);	//not found.

	//Check the disc timer
	pTimerType DiscTimer = pTimerDisciplineReuseStart + spells[spell_id].EndurTimerIndex;
	if(!p_timers.Expired(&database, DiscTimer)) {
		char val1[20]={0};
		char val2[20]={0};
		int32 remain = p_timers.GetRemainingTime(DiscTimer);
		//Message_StringID(0, DISCIPLINE_CANUSEIN, ConvertArray((remain)/60,val1), ConvertArray(remain%60,val2));
		Message(0, "You can use this discipline in %d minutes %d seconds.", ((remain)/60), (remain%60));
		return(false);
	}
	
	//make sure we can use it..
	if(!IsValidSpell(spell_id)) {
		Message(13, "This tome contains invalid knowledge.");
		return(false);
	}
	
	//can we use the spell?
	const SPDat_Spell_Struct &spell = spells[spell_id];
	int8 level_to_use = spell.classes[GetClass() - 1];
	if(level_to_use == 255) {
		Message(13, "Your class cannot learn from this tome.");
		//should summon them a new one...
		return(false);
	}
	
	if(level_to_use > GetLevel()) {
		Message_StringID(13, DISC_LEVEL_USE_ERROR);
		//should summon them a new one...
		return(false);
	}
	
	if(GetEndurance() > spell.EndurCost) {
		SetEndurance(GetEndurance() - spell.EndurCost);
	} else {
		Message(11, "You are too fatigued to use this skill right now.");
		return(false);
	}
	
	CastSpell(spell_id, target, DISCIPLINE_SPELL_SLOT);

	if(spell.recast_time > 0)
	{
		p_timers.Start(DiscTimer, spell.recast_time / 1000);
	}
	return(true);
}

void EntityList::AETaunt(Client* taunter, float range) {
	LinkedListIterator<NPC*> iterator(npc_list);
	
	if(range == 0) {
		range = 100;		//arbitrary default...
	}
	
	range = range * range;
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		NPC * them = iterator.GetData();
		float zdiff = taunter->GetZ() - them->GetZ();
		if (zdiff < 0)
			zdiff *= -1;
		if (zdiff < 10
			&& taunter->IsAttackAllowed(them)
			&& taunter->DistNoRootNoZ(*them) <= range) {
			
			if (taunter->CheckLosFN(them)) {
				taunter->Taunt(them, true);
			}
		}
		iterator.Advance();
	}
}

// solar: causes caster to hit every mob within dist range of center with
// spell_id.
// NPC spells will only affect other NPCs with compatible faction
void EntityList::AESpell(Mob *caster, Mob *center, int16 spell_id, bool affect_caster)
{
	LinkedListIterator<Mob*> iterator(mob_list);
	Mob *curmob;
	
	float dist = caster->GetAOERange(spell_id);
	float dist2 = dist * dist;
	
	bool bad = IsDetrimentalSpell(spell_id);
	bool isnpc = caster->IsNPC();

	const int MAX_TARGETS_ALLOWED = 4;
	int iCounter = 0;
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		curmob = iterator.GetData();
		if(curmob == center)	//do not affect center
			continue;
		if(curmob == caster && !affect_caster)	//watch for caster too
			continue;
		if(center->DistNoRoot(*curmob) > dist2)	//make sure they are in range
			continue;
		if(isnpc && curmob->IsNPC()) {	//check npc->npc casting
			FACTION_VALUE f = curmob->GetReverseFactionCon(caster);
			if(bad) {
				//affect mobs that are on our hate list, or
				//which have bad faction with us
				if( ! (caster->CheckAggro(curmob) || f == FACTION_THREATENLY || f == FACTION_SCOWLS) )
					continue;
			} else {
				//only affect mobs we would assist.
				if( ! (f <= FACTION_AMIABLE))
					continue;
			}
		}
		//finally, make sure they are within range
		if(bad) {
			if(!caster->IsAttackAllowed(curmob))
				continue;
			if(!center->CheckLosFN(curmob))
				continue;
		}
		else {
			if(!caster->IsBeneficialAllowed(curmob))
				continue;
		}

		//if we get here... cast the spell.
		if(IsTargetableAESpell(spell_id) && bad) {
			if(iCounter < MAX_TARGETS_ALLOWED)
				caster->SpellOnTarget(spell_id, curmob);
		}
		else
			caster->SpellOnTarget(spell_id, curmob);

		if(!isnpc) //npcs are not target limited...
			iCounter++;
	}	
}

// solar: causes caster to hit every mob within dist range of center with
// a bard pulse of spell_id.
// NPC spells will only affect other NPCs with compatible faction
void EntityList::AEBardPulse(Mob *caster, Mob *center, int16 spell_id, bool affect_caster)
{
	LinkedListIterator<Mob*> iterator(mob_list);
	Mob *curmob;
	
	float dist = caster->GetAOERange(spell_id);
	float dist2 = dist * dist;
	
	bool bad = IsDetrimentalSpell(spell_id);
	bool isnpc = caster->IsNPC();
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		curmob = iterator.GetData();
		if(curmob == center)	//do not affect center
			continue;
		if(curmob == caster && !affect_caster)	//watch for caster too
			continue;
		if(center->DistNoRoot(*curmob) > dist2)	//make sure they are in range
			continue;
		if(isnpc && curmob->IsNPC()) {	//check npc->npc casting
			FACTION_VALUE f = curmob->GetReverseFactionCon(caster);
			if(bad) {
				//affect mobs that are on our hate list, or
				//which have bad faction with us
				if( ! (caster->CheckAggro(curmob) || f == FACTION_THREATENLY || f == FACTION_SCOWLS) )
					continue;
			} else {
				//only affect mobs we would assist.
				if( ! (f <= FACTION_AMIABLE))
					continue;
			}
		}
		//finally, make sure they are within range
		if(bad) {
			if(!center->CheckLosFN(curmob))
				continue;
		}
		//if we get here... cast the spell.
		curmob->BardPulse(spell_id, caster);
	}
	if(caster->IsClient())
		caster->CastToClient()->CheckSongSkillIncrease(spell_id);
}

//Dook- Rampage and stuff for clients.
//NPCs handle it differently in Mob::Rampage
void EntityList::AEAttack(Mob *attacker, float dist, int Hand, int count) {
//Dook- Will need tweaking, currently no pets or players or horses 
	LinkedListIterator<Mob*> iterator(mob_list); 
	Mob *curmob; 
	
	float dist2 = dist * dist;
	
	int hit = 0;
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance()) { 
		curmob = iterator.GetData(); 
		if(curmob->IsNPC()
			&& curmob != attacker //this is not needed unless NPCs can use this
			&&(attacker->IsAttackAllowed(curmob))
			&& curmob->GetRace() != 216 /* dont attack horses */
			&& (curmob->DistNoRoot(*attacker) <= dist2)
		) {
			attacker->Attack(curmob, Hand); 
			hit++;
			if(count != 0 && hit >= count)
				return;
		}
	}    
} 


