#pragma once
#include "classData.h"

// =============================================================================
//  wowSpellData.h
//
//  Hardcoded spell IDs for every specialization in World of Warcraft.
//  Spell IDs sourced from Wowhead / live game data (current as of TWW).
//
//  Vectors are organized as:
//    rotSpells  — core rotation abilities used every fight
//    offSpells  — offensive cooldowns (damage amplifiers, execute windows, etc.)
//    defSpells  — defensive cooldowns (damage reduction, self-heals, immunities)
//    mobSpells  — mobility / repositioning abilities
//    utilSpells — utility: interrupts, CC, dispels, taunts, raid buffs, etc.
//
// =============================================================================


// ─────────────────────────────────────────────────────────────────────────────
//  WARRIOR
// ─────────────────────────────────────────────────────────────────────────────

// Arms Warrior
inline WoWclass createArmsWarrior() {
    WoWclass c;
    // Mortal Strike, Colossus Smash, Overpower, Slam, Execute, Cleave, Whirlwind, Skullsplitter
    c.setRotSpells({12294, 167105, 7384, 1464, 163201, 845, 1680, 260643});
    // Avatar, Bladestorm, Warbreaker, Spear of Bastion
    c.setOffSpells({107574, 227847, 262161, 307865});
    // Die by the Sword, Shield Wall, Rallying Cry, Defensive Stance, Spell Reflect
    c.setDefSpells({118038, 871, 97462, 386208, 23920});
    // Charge, Heroic Leap, Intervene, Hamstring (kiting)
    c.setMobSpells({100, 6544, 3411, 1715});
    // Intimidating Shout, Piercing Howl, Shattering Throw, Taunt
    c.setUtilSpells({107566, 12323, 64382, 355});
    return c;
}

// Fury Warrior
inline WoWclass createFuryWarrior() {
    WoWclass c;
    // Bloodthirst, Raging Blow, Rampage, Whirlwind, Execute, Crushing Blow, Onslaught
    c.setRotSpells({23881, 85288, 184367, 190411, 163201, 335097, 315720});
    // Recklessness, Avatar, Odyn's Fury, Spear of Bastion
    c.setOffSpells({1719, 107574, 385059, 307865});
    // Enraged Regeneration, Rallying Cry, Spell Reflect, Defensive Stance
    c.setDefSpells({184364, 97462, 23920, 386208});
    // Charge, Heroic Leap, Intervene
    c.setMobSpells({100, 6544, 3411});
    // Intimidating Shout, Piercing Howl, Shattering Throw, Taunt
    c.setUtilSpells({107566, 12323, 64382, 355});
    return c;
}

// Protection Warrior
inline WoWclass createProtWarrior() {
    WoWclass c;
    // Shield Slam, Revenge, Thunder Clap, Devastate, Execute, Shockwave (AoE)
    c.setRotSpells({23922, 6572, 6343, 20243, 163201, 46968});
    // Avatar, Demoralizing Shout, Spear of Bastion
    c.setOffSpells({107574, 1160, 307865});
    // Shield Block, Last Stand, Shield Wall, Spell Reflect, Ignore Pain, Rallying Cry
    c.setDefSpells({2565, 12975, 871, 23920, 190456, 97462});
    // Charge, Heroic Leap, Intervene
    c.setMobSpells({100, 6544, 3411});
    // Taunt, Challenging Shout (AoE taunt), Intimidating Shout, Piercing Howl
    c.setUtilSpells({355, 1161, 107566, 12323});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  PALADIN
// ─────────────────────────────────────────────────────────────────────────────

// Holy Paladin
inline WoWclass createHolyPaladin() {
    WoWclass c;
    // Holy Shock, Flash of Light, Holy Light, Word of Glory, Light of Dawn, Judgment, Crusader Strike
    c.setRotSpells({20473, 19750, 82326, 85673, 85222, 20271, 35395});
    // Avenging Wrath, Holy Avenger, Divine Toll
    c.setOffSpells({31884, 105809, 375576});
    // Divine Shield, Lay on Hands, Blessing of Protection, Blessing of Sacrifice, Ardent Defender (borrowed)
    c.setDefSpells({642, 633, 1022, 6940, 31850});
    // Blessing of Freedom, Divine Steed, Hammer of Justice
    c.setMobSpells({1044, 190784, 853});
    // Rebuke (interrupt), Cleanse, Aura Mastery, Devotion Aura, Turn Evil
    c.setUtilSpells({96231, 4987, 31821, 465, 10326});
    return c;
}

// Protection Paladin
inline WoWclass createProtPaladin() {
    WoWclass c;
    // Shield of the Righteous, Avenger's Shield, Hammer of the Righteous, Consecration, Judgment, Blessed Hammer
    c.setRotSpells({53600, 31935, 53595, 26573, 20271, 204019});
    // Avenging Wrath, Guardian of Ancient Kings, Divine Toll
    c.setOffSpells({31884, 86659, 375576});
    // Divine Shield, Ardent Defender, Lay on Hands, Blessing of Protection, Sentinel
    c.setDefSpells({642, 31850, 633, 1022, 389539});
    // Divine Steed, Blessing of Freedom, Hammer of Justice
    c.setMobSpells({190784, 1044, 853});
    // Rebuke (interrupt), Taunt, Cleanse, Aura Mastery, Devotion Aura, Turn Evil
    c.setUtilSpells({96231, 62124, 4987, 31821, 465, 10326});
    return c;
}

// Retribution Paladin
inline WoWclass createRetPaladin() {
    WoWclass c;
    // Crusader Strike, Templar's Verdict, Divine Storm, Judgment, Blade of Justice, Consecration, Hammer of Wrath, Wake of Ashes
    c.setRotSpells({35395, 85256, 53385, 20271, 184575, 26573, 24275, 255937});
    // Avenging Wrath, Execution Sentence, Final Reckoning
    c.setOffSpells({31884, 343527, 343721});
    // Divine Shield, Lay on Hands, Blessing of Protection, Word of Glory
    c.setDefSpells({642, 633, 1022, 85673});
    // Divine Steed, Blessing of Freedom, Hammer of Justice
    c.setMobSpells({190784, 1044, 853});
    // Rebuke (interrupt), Cleanse, Devotion Aura, Turn Evil
    c.setUtilSpells({96231, 4987, 465, 10326});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  HUNTER
// ─────────────────────────────────────────────────────────────────────────────

// Beast Mastery Hunter
inline WoWclass createBMHunter() {
    WoWclass c;
    // Kill Command, Barbed Shot, Cobra Shot, Multi-Shot, Beast Cleave trigger, Kill Shot
    c.setRotSpells({34026, 217200, 193455, 2643, 187707, 320976});
    // Bestial Wrath, Call of the Wild, Aspect of the Wild
    c.setOffSpells({19574, 359844, 193530});
    // Exhilaration, Survival of the Fittest, Fortitude of the Bear (pet), Aspect of the Turtle
    c.setDefSpells({109304, 264735, 388035, 187650});
    // Disengage, Aspect of the Cheetah, Posthaste
    c.setMobSpells({781, 186257, 109215});
    // Counter Shot (interrupt), Intimidation (stun), Misdirection, Freezing Trap, Tar Trap, Hunter's Mark
    c.setUtilSpells({147362, 19577, 34477, 14309, 187698, 257284});
    return c;
}

// Marksmanship Hunter
inline WoWclass createMMHunter() {
    WoWclass c;
    // Aimed Shot, Arcane Shot, Rapid Fire, Steady Shot, Multi-Shot, Kill Shot, Trick Shots
    c.setRotSpells({19434, 185358, 257044, 56641, 2643, 320976, 257621});
    // Trueshot, Volley, Salvo
    c.setOffSpells({288613, 260243, 400456});
    // Exhilaration, Aspect of the Turtle, Survival of the Fittest
    c.setDefSpells({109304, 187650, 264735});
    // Disengage, Aspect of the Cheetah, Posthaste
    c.setMobSpells({781, 186257, 109215});
    // Counter Shot (interrupt), Intimidation (stun), Misdirection, Freezing Trap, Tar Trap, Hunter's Mark
    c.setUtilSpells({147362, 19577, 34477, 14309, 187698, 257284});
    return c;
}

// Survival Hunter
inline WoWclass createSurvivalHunter() {
    WoWclass c;
    // Raptor Strike, Wildfire Bomb, Mongoose Bite, Flanking Strike, Carve, Kill Shot, Serpent Sting
    c.setRotSpells({186270, 259495, 259387, 269751, 187708, 320976, 271788});
    // Coordinated Assault, Aspect of the Eagle, Death Chakram
    c.setOffSpells({360952, 186289, 375893});
    // Exhilaration, Aspect of the Turtle, Survival of the Fittest
    c.setDefSpells({109304, 187650, 264735});
    // Disengage, Aspect of the Cheetah, Harpoon
    c.setMobSpells({781, 186257, 190925});
    // Muzzle (interrupt), Intimidation (stun), Misdirection, Freezing Trap, Tar Trap, Binding Shot, Hunter's Mark
    c.setUtilSpells({187707, 19577, 34477, 14309, 187698, 109248, 257284});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  ROGUE
// ─────────────────────────────────────────────────────────────────────────────

// Assassination Rogue
inline WoWclass createAssassinationRogue() {
    WoWclass c;
    // Mutilate, Envenom, Garrote, Rupture, Fan of Knives, Crimson Tempest, Shiv
    c.setRotSpells({1329, 32645, 703, 1943, 51723, 121411, 5938});
    // Deathmark, Vendetta, Kingsbane, Sepsis
    c.setOffSpells({360194, 79140, 385627, 328305});
    // Evasion, Cloak of Shadows, Feint, Crimson Vial
    c.setDefSpells({5277, 31224, 1966, 185311});
    // Sprint, Shadowstep, Vanish
    c.setMobSpells({2983, 36554, 1856});
    // Kick (interrupt), Blind, Sap, Tricks of the Trade, Shroud of Concealment
    c.setUtilSpells({1766, 2094, 6770, 57934, 114018});
    return c;
}

// Outlaw Rogue
inline WoWclass createOutlawRogue() {
    WoWclass c;
    // Sinister Strike, Dispatch, Blade Flurry, Pistol Shot, Roll the Bones, Between the Eyes, Ambush
    c.setRotSpells({193315, 2098, 13877, 185763, 315341, 199804, 8676});
    // Adrenaline Rush, Ghostly Strike, Blade Rush, Keep it Rolling
    c.setOffSpells({13750, 196937, 271877, 394400});
    // Evasion, Cloak of Shadows, Feint, Crimson Vial
    c.setDefSpells({5277, 31224, 1966, 185311});
    // Sprint, Shadowstep, Vanish, Grappling Hook
    c.setMobSpells({2983, 36554, 1856, 195457});
    // Kick (interrupt), Blind, Sap, Tricks of the Trade, Shroud of Concealment
    c.setUtilSpells({1766, 2094, 6770, 57934, 114018});
    return c;
}

// Subtlety Rogue
inline WoWclass createSubtletyRogue() {
    WoWclass c;
    // Shadowstrike, Backstab, Eviscerate, Rupture, Shadow Dance, Symbols of Death, Black Powder, Shuriken Storm
    c.setRotSpells({185438, 53, 196819, 1943, 185313, 212283, 319175, 197835});
    // Secret Technique, Shadow Blades, Flagellation
    c.setOffSpells({280719, 121471, 384631});
    // Evasion, Cloak of Shadows, Feint, Crimson Vial
    c.setDefSpells({5277, 31224, 1966, 185311});
    // Sprint, Shadowstep, Vanish
    c.setMobSpells({2983, 36554, 1856});
    // Kick (interrupt), Blind, Sap, Tricks of the Trade, Shroud of Concealment
    c.setUtilSpells({1766, 2094, 6770, 57934, 114018});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  PRIEST
// ─────────────────────────────────────────────────────────────────────────────

// Discipline Priest
inline WoWclass createDiscPriest() {
    WoWclass c;
    // Penance, Power Word: Shield, Flash Heal, Atonement apply (Plea/Shadowmend), Radiance, Smite, Power Word: Radiance
    c.setRotSpells({47540, 17, 2061, 186263, 194509, 585, 194509});
    // Rapture, Evangelism, Spirit Shell, Mindbender, Power Word: Barrier
    c.setOffSpells({47536, 246287, 109964, 123040, 62618});
    // Pain Suppression, Power Word: Barrier, Desperate Prayer, Fade
    c.setDefSpells({33206, 62618, 19236, 586});
    // Leap of Faith, Body and Soul (speed from PW:S)
    c.setMobSpells({73325, 64901});
    // Purify (dispel), Mass Dispel, Psychic Scream, Power Infusion, Mind Control
    c.setUtilSpells({527, 32375, 8122, 10060, 605});
    return c;
}

// Holy Priest
inline WoWclass createHolyPriest() {
    WoWclass c;
    // Flash Heal, Heal, Prayer of Healing, Holy Word: Serenity, Holy Word: Sanctify, Circle of Healing, Renew, Guardian Spirit
    c.setRotSpells({2061, 2060, 596, 2050, 34861, 64843, 139, 47788});
    // Divine Hymn, Apotheosis, Holy Word: Salvation
    c.setOffSpells({64843, 200183, 265202});
    // Guardian Spirit, Desperate Prayer, Fade
    c.setDefSpells({47788, 19236, 586});
    // Leap of Faith, Body and Soul
    c.setMobSpells({73325, 64901});
    // Purify (dispel), Mass Dispel, Psychic Scream, Power Infusion, Shackle Undead
    c.setUtilSpells({527, 32375, 8122, 10060, 9484});
    return c;
}

// Shadow Priest
inline WoWclass createShadowPriest() {
    WoWclass c;
    // Mind Blast, Shadow Word: Pain, Vampiric Touch, Devouring Plague, Void Bolt, Mind Flay, Shadow Word: Death, Mind Sear
    c.setRotSpells({8092, 589, 34914, 335467, 205448, 15407, 32379, 48045});
    // Void Eruption, Dark Ascension, Mindbender, Shadowfiend
    c.setOffSpells({228260, 391109, 123040, 34433});
    // Dispersion, Vampiric Embrace, Fade, Desperate Prayer
    c.setDefSpells({47585, 15286, 586, 19236});
    // Leap of Faith, Body and Soul
    c.setMobSpells({73325, 64901});
    // Silence (interrupt/silence), Dispel Magic, Mass Dispel, Psychic Scream, Mind Control
    c.setUtilSpells({15487, 528, 32375, 8122, 605});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  DEATH KNIGHT
// ─────────────────────────────────────────────────────────────────────────────

// Blood Death Knight
inline WoWclass createBloodDK() {
    WoWclass c;
    // Death Strike, Heart Strike, Marrowrend, Blood Boil, Consumption, Death's Caress
    c.setRotSpells({49998, 206930, 195182, 50842, 274156, 195292});
    // Dancing Rune Weapon, Tombstone, Abomination Limb
    c.setOffSpells({49028, 219809, 383269});
    // Vampiric Blood, Icebound Fortitude, Anti-Magic Shell, Anti-Magic Zone, Purgatory
    c.setDefSpells({55233, 48792, 48707, 51052, 114556});
    // Death Grip, Death's Advance, Wraith Walk
    c.setMobSpells({49576, 48265, 212552});
    // Mind Freeze (interrupt), Dark Command (taunt), Chains of Ice, Strangulate, Raise Ally, Control Undead
    c.setUtilSpells({47528, 56222, 45524, 47476, 61999, 111673});
    return c;
}

// Frost Death Knight
inline WoWclass createFrostDK() {
    WoWclass c;
    // Obliterate, Frost Strike, Howling Blast, Remorseless Winter, Frostwyrm's Fury, Glacial Advance
    c.setRotSpells({49020, 49143, 237680, 196770, 279302, 194913});
    // Pillar of Frost, Empower Rune Weapon, Breath of Sindragosa, Abomination Limb
    c.setOffSpells({51271, 47568, 152279, 383269});
    // Icebound Fortitude, Anti-Magic Shell, Anti-Magic Zone
    c.setDefSpells({48792, 48707, 51052});
    // Death Grip, Death's Advance, Wraith Walk
    c.setMobSpells({49576, 48265, 212552});
    // Mind Freeze (interrupt), Chains of Ice, Strangulate, Raise Ally, Control Undead
    c.setUtilSpells({47528, 45524, 47476, 61999, 111673});
    return c;
}

// Unholy Death Knight
inline WoWclass createUnholyDK() {
    WoWclass c;
    // Scourge Strike, Festering Strike, Death Coil, Epidemic, Apocalypse, Dark Transformation, Army of the Dead
    c.setRotSpells({55090, 85948, 47541, 212417, 275699, 63560, 42650});
    // Unholy Assault, Summon Gargoyle, Abomination Limb
    c.setOffSpells({207289, 49206, 383269});
    // Icebound Fortitude, Anti-Magic Shell, Anti-Magic Zone
    c.setDefSpells({48792, 48707, 51052});
    // Death Grip, Death's Advance, Wraith Walk
    c.setMobSpells({49576, 48265, 212552});
    // Mind Freeze (interrupt), Chains of Ice, Strangulate, Raise Ally, Control Undead
    c.setUtilSpells({47528, 45524, 47476, 61999, 111673});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  SHAMAN
// ─────────────────────────────────────────────────────────────────────────────

// Elemental Shaman
inline WoWclass createElementalShaman() {
    WoWclass c;
    // Lightning Bolt, Lava Burst, Flame Shock, Earth Shock, Earthquake, Elemental Blast, Icefury, Frost Shock
    c.setRotSpells({188196, 51505, 188389, 8042, 61882, 117014, 210714, 196840});
    // Storm Elemental, Stormkeeper, Ascendance, Liquid Magma Totem
    c.setOffSpells({192249, 191634, 114051, 192222});
    // Astral Shift, Earth Elemental, Healing Surge
    c.setDefSpells({108271, 198103, 8004});
    // Ghost Wolf, Gust of Wind, Wind Rush Totem
    c.setMobSpells({2645, 192063, 192077});
    // Wind Shear (interrupt), Hex, Purge, Tremor Totem, Capacitor Totem, Grounding Totem
    c.setUtilSpells({57994, 51514, 370, 8143, 192058, 204336});
    return c;
}

// Enhancement Shaman
inline WoWclass createEnhancementShaman() {
    WoWclass c;
    // Stormstrike, Lava Lash, Crash Lightning, Flame Shock, Lightning Bolt, Frost Shock, Ice Strike, Sundering
    c.setRotSpells({17364, 60103, 187874, 188389, 188196, 196840, 342240, 197214});
    // Feral Spirit, Ascendance, Doom Winds, Vesper Totem
    c.setOffSpells({51533, 114051, 384352, 324386});
    // Astral Shift, Earth Elemental, Healing Surge
    c.setDefSpells({108271, 198103, 8004});
    // Ghost Wolf, Gust of Wind
    c.setMobSpells({2645, 192063});
    // Wind Shear (interrupt), Hex, Purge, Tremor Totem, Capacitor Totem, Grounding Totem
    c.setUtilSpells({57994, 51514, 370, 8143, 192058, 204336});
    return c;
}

// Restoration Shaman
inline WoWclass createRestoShaman() {
    WoWclass c;
    // Healing Wave, Healing Surge, Chain Heal, Riptide, Healing Rain, Wellspring, Unleash Life
    c.setRotSpells({77472, 8004, 1064, 61295, 73920, 197995, 73685});
    // Ascendance, Healing Tide Totem, Spirit Link Totem, Earthen Wall Totem
    c.setOffSpells({114052, 108280, 98008, 198838});
    // Astral Shift, Earth Elemental, Nature's Guardian
    c.setDefSpells({108271, 198103, 30884});
    // Ghost Wolf, Gust of Wind, Wind Rush Totem
    c.setMobSpells({2645, 192063, 192077});
    // Wind Shear (interrupt), Hex, Purge, Tremor Totem, Capacitor Totem, Grounding Totem, Mana Tide Totem
    c.setUtilSpells({57994, 51514, 370, 8143, 192058, 204336, 16190});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  MAGE
// ─────────────────────────────────────────────────────────────────────────────

// Arcane Mage
inline WoWclass createArcaneMage() {
    WoWclass c;
    // Arcane Blast, Arcane Missiles, Arcane Barrage, Arcane Explosion, Arcane Surge, Touch of the Magi
    c.setRotSpells({30451, 5143, 44425, 1449, 365350, 321507});
    // Arcane Power, Evocation, Presence of Mind, Siphon Storm
    c.setOffSpells({12042, 12051, 12043, 384274});
    // Ice Block, Alter Time, Prismatic Barrier, Mirror Image
    c.setDefSpells({45438, 342245, 235450, 55342});
    // Blink, Shimmer, Slow Fall
    c.setMobSpells({1953, 212653, 130});
    // Counterspell (interrupt), Polymorph, Spellsteal, Remove Curse, Time Warp, Frost Nova
    c.setUtilSpells({2139, 118, 30449, 475, 80353, 122});
    return c;
}

// Fire Mage
inline WoWclass createFireMage() {
    WoWclass c;
    // Fireball, Pyroblast, Fire Blast, Scorch, Phoenix Flames, Flamestrike, Dragon's Breath
    c.setRotSpells({133, 11366, 108853, 2948, 257541, 2120, 31661});
    // Combustion, Meteor, Sun King's Blessing trigger
    c.setOffSpells({190319, 153561, 383886});
    // Ice Block, Alter Time, Cauterize, Mirror Image
    c.setDefSpells({45438, 342245, 86949, 55342});
    // Blink, Shimmer, Blazing Speed
    c.setMobSpells({1953, 212653, 235219});
    // Counterspell (interrupt), Polymorph, Spellsteal, Remove Curse, Time Warp, Frost Nova
    c.setUtilSpells({2139, 118, 30449, 475, 80353, 122});
    return c;
}

// Frost Mage
inline WoWclass createFrostMage() {
    WoWclass c;
    // Frostbolt, Ice Lance, Frozen Orb, Blizzard, Flurry, Ebonbolt, Cold Snap
    c.setRotSpells({116, 30455, 84714, 190356, 44614, 257537, 235219});
    // Icy Veins, Ray of Frost, Shifting Power
    c.setOffSpells({12472, 205021, 382440});
    // Ice Block, Alter Time, Ice Barrier, Mirror Image
    c.setDefSpells({45438, 342245, 11426, 55342});
    // Blink, Shimmer, Cone of Cold (slow)
    c.setMobSpells({1953, 212653, 120});
    // Counterspell (interrupt), Polymorph, Spellsteal, Remove Curse, Time Warp, Frost Nova
    c.setUtilSpells({2139, 118, 30449, 475, 80353, 122});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  WARLOCK
// ─────────────────────────────────────────────────────────────────────────────

// Affliction Warlock
inline WoWclass createAfflictionWarlock() {
    WoWclass c;
    // Unstable Affliction, Agony, Corruption, Haunt, Shadow Bolt, Malefic Rapture, Vile Taint, Phantom Singularity
    c.setRotSpells({30108, 980, 172, 48181, 686, 324536, 278350, 205179});
    // Summon Darkglare, Soul Rot, Inevitable Demise
    c.setOffSpells({205180, 325640, 334319});
    // Unending Resolve, Dark Pact, Drain Life, Soulstone
    c.setDefSpells({104773, 108416, 234153, 20707});
    // Demonic Circle: Teleport, Burning Rush
    c.setMobSpells({48020, 111400});
    // Spell Lock (pet interrupt), Shadowfury, Banish, Create Healthstone, Curse of Weakness
    c.setUtilSpells({19647, 30283, 710, 6201, 702});
    return c;
}

// Demonology Warlock
inline WoWclass createDemoWarlock() {
    WoWclass c;
    // Hand of Gul'dan, Call Dreadstalkers, Demonbolt, Shadow Bolt, Implosion, Power Siphon, Bilescourge Bombers
    c.setRotSpells({105174, 104316, 264178, 686, 196277, 264803, 267211});
    // Nether Portal, Grimoire: Felguard, Summon Demonic Tyrant, Dark Glare
    c.setOffSpells({267217, 111898, 265187, 205180});
    // Unending Resolve, Dark Pact, Drain Life, Soulstone
    c.setDefSpells({104773, 108416, 234153, 20707});
    // Demonic Circle: Teleport, Burning Rush
    c.setMobSpells({48020, 111400});
    // Spell Lock (pet interrupt), Shadowfury, Banish, Create Healthstone, Curse of Weakness
    c.setUtilSpells({19647, 30283, 710, 6201, 702});
    return c;
}

// Destruction Warlock
inline WoWclass createDestroWarlock() {
    WoWclass c;
    // Incinerate, Chaos Bolt, Conflagrate, Immolate, Rain of Fire, Shadowburn, Channel Demonfire
    c.setRotSpells({29722, 116858, 17962, 348, 5740, 17877, 196447});
    // Summon Infernal, Dark Soul: Instability, Havoc
    c.setOffSpells({1122, 113858, 80240});
    // Unending Resolve, Dark Pact, Drain Life, Soulstone
    c.setDefSpells({104773, 108416, 234153, 20707});
    // Demonic Circle: Teleport, Burning Rush
    c.setMobSpells({48020, 111400});
    // Spell Lock (pet interrupt), Shadowfury, Banish, Create Healthstone, Curse of Weakness
    c.setUtilSpells({19647, 30283, 710, 6201, 702});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  MONK
// ─────────────────────────────────────────────────────────────────────────────

// Brewmaster Monk
inline WoWclass createBrewmasterMonk() {
    WoWclass c;
    // Keg Smash, Blackout Kick, Breath of Fire, Tiger Palm, Spinning Crane Kick, Rushing Jade Wind
    c.setRotSpells({121253, 100784, 115181, 100787, 101546, 116847});
    // Invoke Niuzao, Bonedust Brew, Weapons of Order
    c.setOffSpells({132578, 325216, 387184});
    // Ironskin Brew, Purifying Brew, Celestial Brew, Fortifying Brew, Dampen Harm
    c.setDefSpells({115308, 119582, 322507, 115203, 122278});
    // Roll, Chi Torpedo, Ring of Peace
    c.setMobSpells({109132, 115008, 116844});
    // Spear Hand Strike (interrupt), Leg Sweep (AoE stun), Paralysis, Detox, Tiger's Lust, Provoke (taunt)
    c.setUtilSpells({116705, 119381, 115078, 115450, 116841, 116189});
    return c;
}

// Mistweaver Monk
inline WoWclass createMistweaverMonk() {
    WoWclass c;
    // Vivify, Enveloping Mist, Renewing Mist, Rising Sun Kick, Essence Font, Chi Burst, Soothing Mist
    c.setRotSpells({116670, 124682, 115151, 107428, 191837, 123986, 115175});
    // Invoke Chi-Ji, Thunder Focus Tea, Revival, Restoral
    c.setOffSpells({325197, 116680, 115310, 388615});
    // Life Cocoon, Fortifying Brew, Diffuse Magic
    c.setDefSpells({116849, 115203, 122783});
    // Roll, Chi Torpedo, Transcendence
    c.setMobSpells({109132, 115008, 101643});
    // Spear Hand Strike (interrupt), Leg Sweep (AoE stun), Paralysis, Detox, Tiger's Lust
    c.setUtilSpells({116705, 119381, 115078, 115450, 116841});
    return c;
}

// Windwalker Monk
inline WoWclass createWindwalkerMonk() {
    WoWclass c;
    // Rising Sun Kick, Fists of Fury, Blackout Kick, Tiger Palm, Spinning Crane Kick, Strike of the Windlord, Whirling Dragon Punch
    c.setRotSpells({107428, 113656, 100784, 100787, 101546, 392983, 152175});
    // Storm, Earth, and Fire, Serenity, Weapons of Order, Touch of Death
    c.setOffSpells({137639, 152173, 387184, 322109});
    // Fortifying Brew, Diffuse Magic, Dampen Harm, Karma
    c.setDefSpells({115203, 122783, 122278, 122470});
    // Roll, Chi Torpedo, Flying Serpent Kick
    c.setMobSpells({109132, 115008, 101545});
    // Spear Hand Strike (interrupt), Leg Sweep (AoE stun), Paralysis, Detox, Tiger's Lust
    c.setUtilSpells({116705, 119381, 115078, 115450, 116841});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  DRUID
// ─────────────────────────────────────────────────────────────────────────────

// Balance Druid
inline WoWclass createBalanceDruid() {
    WoWclass c;
    // Starsurge, Starfire, Wrath, Moonfire, Sunfire, Starfall, Full Moon, Fury of Elune
    c.setRotSpells({78674, 194153, 190984, 8921, 93402, 191034, 274281, 202770});
    // Celestial Alignment, Incarnation: Chosen of Elune, Convoke the Spirits
    c.setOffSpells({194223, 102560, 391528});
    // Barkskin, Survival Instincts, Renewal, Bear Form
    c.setDefSpells({22812, 61336, 108238, 5487});
    // Dash, Wild Charge, Typhoon
    c.setMobSpells({1850, 102401, 132469});
    // Innervate, Rebirth, Entangling Roots, Cyclone, Remove Corruption, Stampeding Roar
    c.setUtilSpells({29166, 20484, 339, 33786, 2782, 106898});
    return c;
}

// Feral Druid
inline WoWclass createFeralDruid() {
    WoWclass c;
    // Shred, Rake, Rip, Ferocious Bite, Thrash, Swipe, Brutal Slash, Primal Wrath
    c.setRotSpells({5221, 1822, 1079, 22568, 106830, 213764, 202028, 285381});
    // Tiger's Fury, Berserk, Incarnation: King of the Jungle, Convoke the Spirits
    c.setOffSpells({5217, 106951, 102543, 391528});
    // Barkskin, Survival Instincts, Renewal
    c.setDefSpells({22812, 61336, 108238});
    // Dash, Wild Charge, Skull Bash
    c.setMobSpells({1850, 102401, 106839});
    // Innervate, Rebirth, Entangling Roots, Cyclone, Remove Corruption, Stampeding Roar
    c.setUtilSpells({29166, 20484, 339, 33786, 2782, 106898});
    return c;
}

// Guardian Druid
inline WoWclass createGuardianDruid() {
    WoWclass c;
    // Mangle, Thrash, Swipe, Maul, Moonfire, Pulverize
    c.setRotSpells({33917, 77758, 213764, 6807, 8921, 80313});
    // Incarnation: Guardian of Ursoc, Berserk, Convoke the Spirits
    c.setOffSpells({102558, 50334, 391528});
    // Barkskin, Survival Instincts, Frenzied Regeneration, Ironfur, Renewal
    c.setDefSpells({22812, 61336, 22842, 192081, 108238});
    // Dash, Wild Charge, Skull Bash
    c.setMobSpells({1850, 102401, 106839});
    // Growl (taunt), Innervate, Rebirth, Entangling Roots, Cyclone, Remove Corruption, Stampeding Roar
    c.setUtilSpells({6795, 29166, 20484, 339, 33786, 2782, 106898});
    return c;
}

// Restoration Druid
inline WoWclass createRestoDruid() {
    WoWclass c;
    // Regrowth, Rejuvenation, Wild Growth, Lifebloom, Efflorescence, Swiftmend, Tranquility, Cenarion Ward
    c.setRotSpells({8936, 774, 48438, 33763, 145205, 18562, 740, 102351});
    // Incarnation: Tree of Life, Flourish, Convoke the Spirits
    c.setOffSpells({33891, 197721, 391528});
    // Barkskin, Survival Instincts, Renewal
    c.setDefSpells({22812, 61336, 108238});
    // Dash, Wild Charge, Ursol's Vortex
    c.setMobSpells({1850, 102401, 102793});
    // Innervate, Rebirth, Entangling Roots, Cyclone, Remove Corruption, Stampeding Roar
    c.setUtilSpells({29166, 20484, 339, 33786, 2782, 106898});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  DEMON HUNTER
// ─────────────────────────────────────────────────────────────────────────────

// Havoc Demon Hunter
inline WoWclass createHavocDH() {
    WoWclass c;
    // Chaos Strike, Blade Dance, Demon's Bite, Immolation Aura, Eye Beam, Death Sweep, Throw Glaive, Annihilation
    c.setRotSpells({162794, 188499, 185123, 258920, 198013, 210152, 185123, 201427});
    // Metamorphosis, Essence Break, The Hunt, Sinful Brand
    c.setOffSpells({191427, 258860, 370965, 317009});
    // Blur, Darkness, Netherwalk, Fel Barrage
    c.setDefSpells({198589, 196718, 196555, 258925});
    // Fel Rush, Vengeful Retreat, Consume Magic
    c.setMobSpells({195072, 198793, 278326});
    // Disrupt (interrupt), Imprison, Chaos Nova (AoE stun), Sigil of Silence, Sigil of Misery
    c.setUtilSpells({183752, 217832, 179057, 202137, 207684});
    return c;
}

// Vengeance Demon Hunter
inline WoWclass createVengeanceDH() {
    WoWclass c;
    // Soul Cleave, Immolation Aura, Sigil of Flame, Shear, Fracture, Fel Devastation, Spirit Bomb, Felblade
    c.setRotSpells({228477, 258920, 204596, 203782, 263642, 212084, 247454, 232893});
    // Metamorphosis, The Hunt, Fiery Brand
    c.setOffSpells({187827, 370965, 204021});
    // Demon Spikes, Darkness, Metamorphosis, Soul Barrier
    c.setDefSpells({203720, 196718, 187827, 263648});
    // Infernal Strike, Fel Rush
    c.setMobSpells({189110, 195072});
    // Disrupt (interrupt), Torment (taunt), Imprison, Sigil of Silence, Sigil of Misery, Consume Magic
    c.setUtilSpells({183752, 185245, 217832, 202137, 207684, 278326});
    return c;
}


// ─────────────────────────────────────────────────────────────────────────────
//  EVOKER
// ─────────────────────────────────────────────────────────────────────────────

// Devastation Evoker
inline WoWclass createDevastationEvoker() {
    WoWclass c;
    // Disintegrate, Pyre, Fire Breath, Eternity Surge, Living Flame, Azure Strike, Deep Breath
    c.setRotSpells({356995, 357211, 357208, 359073, 361469, 362969, 357210});
    // Dragonrage, Shattering Star, Tip the Scales
    c.setOffSpells({375087, 370452, 370553});
    // Obsidian Scales, Renewing Blaze, Zephyr
    c.setDefSpells({363916, 374251, 374227});
    // Hover, Tail Swipe, Wing Buffet
    c.setMobSpells({358267, 368970, 357214});
    // Quell (interrupt), Oppressing Roar, Rescue, Sleep Walk, Naturalize (dispel)
    c.setUtilSpells({351338, 372048, 370665, 360806, 374349});
    return c;
}

// Preservation Evoker
inline WoWclass createPreservationEvoker() {
    WoWclass c;
    // Emerald Blossom, Spiritbloom, Dream Breath, Living Flame, Echo, Reversion, Verdant Embrace
    c.setRotSpells({355913, 367226, 355936, 361469, 364343, 366155, 360995});
    // Rewind, Temporal Anomaly, Time Dilation, Dream Flight
    c.setOffSpells({363534, 373861, 357170, 376943});
    // Obsidian Scales, Renewing Blaze, Zephyr
    c.setDefSpells({363916, 374251, 374227});
    // Hover, Tail Swipe, Wing Buffet
    c.setMobSpells({358267, 368970, 357214});
    // Quell (interrupt), Oppressing Roar, Rescue, Sleep Walk, Naturalize (dispel)
    c.setUtilSpells({351338, 372048, 370665, 360806, 374349});
    return c;
}

// Augmentation Evoker
inline WoWclass createAugmentationEvoker() {
    WoWclass c;
    // Ebon Might, Prescience, Breath of Eons, Upheaval, Living Flame, Azure Strike, Eruption
    c.setRotSpells({395152, 409311, 403631, 396286, 361469, 362969, 395160});
    // Spatial Paradox, Sands of Time, Fate Mirror
    c.setOffSpells({406732, 395152, 399787});
    // Obsidian Scales, Renewing Blaze, Zephyr
    c.setDefSpells({363916, 374251, 374227});
    // Hover, Tail Swipe, Wing Buffet
    c.setMobSpells({358267, 368970, 357214});
    // Quell (interrupt), Oppressing Roar, Rescue, Sleep Walk, Naturalize (dispel)
    c.setUtilSpells({351338, 372048, 370665, 360806, 374349});
    return c;
}


// =============================================================================
//  CONVENIENCE: build all specs at once into a named struct
// =============================================================================
struct AllSpecs {
    // Warrior
    WoWclass armsWarrior       = createArmsWarrior();
    WoWclass furyWarrior       = createFuryWarrior();
    WoWclass protWarrior       = createProtWarrior();
    // Paladin
    WoWclass holyPaladin       = createHolyPaladin();
    WoWclass protPaladin       = createProtPaladin();
    WoWclass retPaladin        = createRetPaladin();
    // Hunter
    WoWclass bmHunter          = createBMHunter();
    WoWclass mmHunter          = createMMHunter();
    WoWclass survivalHunter    = createSurvivalHunter();
    // Rogue
    WoWclass assassinationRogue = createAssassinationRogue();
    WoWclass outlawRogue       = createOutlawRogue();
    WoWclass subtletyRogue     = createSubtletyRogue();
    // Priest
    WoWclass discPriest        = createDiscPriest();
    WoWclass holyPriest        = createHolyPriest();
    WoWclass shadowPriest      = createShadowPriest();
    // Death Knight
    WoWclass bloodDK           = createBloodDK();
    WoWclass frostDK           = createFrostDK();
    WoWclass unholyDK          = createUnholyDK();
    // Shaman
    WoWclass elementalShaman   = createElementalShaman();
    WoWclass enhancementShaman = createEnhancementShaman();
    WoWclass restoShaman       = createRestoShaman();
    // Mage
    WoWclass arcaneMage        = createArcaneMage();
    WoWclass fireMage          = createFireMage();
    WoWclass frostMage         = createFrostMage();
    // Warlock
    WoWclass afflictionWarlock = createAfflictionWarlock();
    WoWclass demoWarlock       = createDemoWarlock();
    WoWclass destroWarlock     = createDestroWarlock();
    // Monk
    WoWclass brewmasterMonk    = createBrewmasterMonk();
    WoWclass mistweaverMonk    = createMistweaverMonk();
    WoWclass windwalkerMonk    = createWindwalkerMonk();
    // Druid
    WoWclass balanceDruid      = createBalanceDruid();
    WoWclass feralDruid        = createFeralDruid();
    WoWclass guardianDruid     = createGuardianDruid();
    WoWclass restoDruid        = createRestoDruid();
    // Demon Hunter
    WoWclass havocDH           = createHavocDH();
    WoWclass vengeanceDH       = createVengeanceDH();
    // Evoker
    WoWclass devastationEvoker = createDevastationEvoker();
    WoWclass preservationEvoker= createPreservationEvoker();
    WoWclass augmentationEvoker= createAugmentationEvoker();
};