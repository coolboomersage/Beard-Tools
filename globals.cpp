#include "globals.h"

// ---------------------------------------------------------------------------
// Report state
// ---------------------------------------------------------------------------
ReportData        g_report;
std::mutex        g_reportMutex;
std::atomic<bool> g_reportReady{ false };

std::string g_cachedReportCode;
std::string g_cachedBearerToken;

// ---------------------------------------------------------------------------
// Per-player window state
// ---------------------------------------------------------------------------
Fight             g_currentFight;
bool              g_fetchedBossData = false;
std::mutex        g_debugMutex;
bool              g_debugWindowOpen   = false;
std::atomic<bool> g_debugFetching{ false };
std::string       g_debugWindowTitle;
std::string       g_debugRawResponse;
int               g_debugPlayerId = -1;

// ---------------------------------------------------------------------------
// Spec map
// Map to convert from WCL class/spec listings into the code equivalent.
// kSpecMap holds raw pointers into masterSpecList, so both must live here.
// ---------------------------------------------------------------------------
AllSpecs masterSpecList;

const SpecEntry kSpecMap[39] = {
    // Warrior
    { "Warrior",     "Arms",          &masterSpecList.armsWarrior        },
    { "Warrior",     "Fury",          &masterSpecList.furyWarrior        },
    { "Warrior",     "Protection",    &masterSpecList.protWarrior        },
    // Paladin
    { "Paladin",     "Holy",          &masterSpecList.holyPaladin        },
    { "Paladin",     "Protection",    &masterSpecList.protPaladin        },
    { "Paladin",     "Retribution",   &masterSpecList.retPaladin         },
    // Hunter
    { "Hunter",      "BeastMastery",  &masterSpecList.bmHunter           },
    { "Hunter",      "Marksmanship",  &masterSpecList.mmHunter           },
    { "Hunter",      "Survival",      &masterSpecList.survivalHunter     },
    // Rogue
    { "Rogue",       "Assassination", &masterSpecList.assassinationRogue },
    { "Rogue",       "Outlaw",        &masterSpecList.outlawRogue        },
    { "Rogue",       "Subtlety",      &masterSpecList.subtletyRogue      },
    // Priest
    { "Priest",      "Discipline",    &masterSpecList.discPriest         },
    { "Priest",      "Holy",          &masterSpecList.holyPriest         },
    { "Priest",      "Shadow",        &masterSpecList.shadowPriest       },
    // Death Knight
    { "DeathKnight", "Blood",         &masterSpecList.bloodDK            },
    { "DeathKnight", "Frost",         &masterSpecList.frostDK            },
    { "DeathKnight", "Unholy",        &masterSpecList.unholyDK           },
    // Shaman
    { "Shaman",      "Elemental",     &masterSpecList.elementalShaman    },
    { "Shaman",      "Enhancement",   &masterSpecList.enhancementShaman  },
    { "Shaman",      "Restoration",   &masterSpecList.restoShaman        },
    // Mage
    { "Mage",        "Arcane",        &masterSpecList.arcaneMage         },
    { "Mage",        "Fire",          &masterSpecList.fireMage           },
    { "Mage",        "Frost",         &masterSpecList.frostMage          },
    // Warlock
    { "Warlock",     "Affliction",    &masterSpecList.afflictionWarlock  },
    { "Warlock",     "Demonology",    &masterSpecList.demoWarlock        },
    { "Warlock",     "Destruction",   &masterSpecList.destroWarlock      },
    // Monk
    { "Monk",        "Brewmaster",    &masterSpecList.brewmasterMonk     },
    { "Monk",        "Mistweaver",    &masterSpecList.mistweaverMonk     },
    { "Monk",        "Windwalker",    &masterSpecList.windwalkerMonk     },
    // Druid
    { "Druid",       "Balance",       &masterSpecList.balanceDruid       },
    { "Druid",       "Feral",         &masterSpecList.feralDruid         },
    { "Druid",       "Guardian",      &masterSpecList.guardianDruid      },
    { "Druid",       "Restoration",   &masterSpecList.restoDruid         },
    // Demon Hunter
    { "DemonHunter", "Havoc",         &masterSpecList.havocDH            },
    { "DemonHunter", "Vengeance",     &masterSpecList.vengeanceDH        },
    // Evoker
    { "Evoker",      "Devastation",   &masterSpecList.devastationEvoker  },
    { "Evoker",      "Preservation",  &masterSpecList.preservationEvoker },
    { "Evoker",      "Augmentation",  &masterSpecList.augmentationEvoker },
};