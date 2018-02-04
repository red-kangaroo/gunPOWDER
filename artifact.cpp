/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        artifact.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This code manages the list of allocated artifacts, and the code
 *	required to build artifacts.
 */

#include "artifact.h"
#include "rand.h"
#include "grammar.h"

// This is the root artifact pointer.  Disgusting O(N) search for
// each artifact, but likely cheaper than rebuilding from the string.
// Theoritically, total number of artifacts is low.
ARTIFACT *glb_artifacts = 0;

int	  glb_artifactseed = 0;


ARTIFACT::ARTIFACT()
{
    // We want to ensure all of the attacks are fully initialized.
    // This is more difficult than one may think, as their data structure
    // changes when source.txt does.
    // By initializing from safe attacks, new attack bits won't
    // be uninitalized when our build code doesn't know about them.
    memcpy(&attack, &glb_attackdefs[ATTACK_MISUSED], sizeof(ATTACK_DEF));
    memcpy(&thrownattack, &glb_attackdefs[ATTACK_MISTHROWN], sizeof(ATTACK_DEF));

    hasattack = false;
    hasthrownattack = false;

    acbonus = 0;
    lightradius = 0;
    iscarryintrinsic = 0;
    intrinsics = 0;
    name = 0;
    baseitem = ITEM_NONE;
    next = 0;
}

ARTIFACT::~ARTIFACT()
{
    if (intrinsics)
	free(intrinsics);
    if (name)
	free(name);
    delete next;
}

//
// Artifact functions
//

int
artifact_rand()
{
    // This is straight from K & R
    glb_artifactseed = (1103515245 * glb_artifactseed + 12345);
    glb_artifactseed &= 32767;

    return glb_artifactseed;
}

// Seeds the artifact RNG with the given name.
// We don't want to just reseed our main RNG as we don't have
// an efficient seed/unseed procedure.  (getseed will truncate
// the RNG state to 32 bits)
void
artifact_seed(const char *name)
{
    glb_artifactseed = 0;
    while (*name)
    {
	glb_artifactseed *= 37;
	glb_artifactseed += *name;
	name++;
    }

    glb_artifactseed = glb_artifactseed & 32767;

    // We want to iterate the random number generator a few times
    // to remove the highly dependent variables
    // (as we are seeding from a string hash, our initial values
    // are likely strongly correlated)
    // cf: http://home1.gte.net/deleyd/random/random4.html
    artifact_rand();
    artifact_rand();
    artifact_rand();
}

int
artifact_randchoice(int num)
{
    int		v;

    // Choice of 0 or 1 is always 0.
    if (num < 2) return 0;

    v = abs(artifact_rand());
    v %= num;

    return v;
}

bool
artifact_randchance(int percentage)
{
    int		percent;

    percent = artifact_randchoice(100);
    // We want strict less than so percentage 0 will never pass,
    // and percentage 99 will pass only one in 100.
    return percent < percentage;
}

void
artifact_init()
{
    delete glb_artifacts;
    glb_artifacts = 0;
}

BUF
artifact_buildname(ITEM_NAMES /*definition*/)
{
    BUF			buf;

    // A collection of self-serving syllables, often dredged from
    // r.g.r.d.
    // Should divide into different categories.  Some are true syllables
    // (such as "oma") which should be appended together.  Other are
    // miniwords (like "shock") that should be joined with hyphens or
    // mixed case (as in: ShockFrost)
    const char		*syllables[] =
    {
	"shar",
	"frei",
	"fyre",
	"myst",
	"amy",
	"wang",
	"whiz",
	"wor",
	"woo",
	"hoo",
	"loo",
	"brask",
	"trey",
	"klay",
	"oma",
	"upa",
	"relat",
	"froody",
	"bar",
	"baz",
	"zub",
	"foe",
	"fae",
	"ash",
	"xyzzy",
	"were",
	"slag",
	"slog",
	"wyrd",
	"xul",
	"ensis",
	"alph",
	"omeg",
	"xsi",
	0
    };
    
    int			num_syllable, syl;
	
    for (num_syllable = 0; syllables[num_syllable]; num_syllable++);

    // 2-3 syllables.
    // Ideally we should sometimes hyphenate these.
    // This would all, I think, be much easier in German than English.
    
    syl = rand_choice(num_syllable);
    buf.strcpy(syllables[syl]);
    syl = rand_choice(num_syllable);
    buf.strcat(syllables[syl]);
    // A strong effect can be achieved by doubling the last
    // syllable, so increase the probability of that (Orcsmashsmash)
    if (!rand_choice(3))
    {
	if (rand_choice(3))
	    buf.strcat(syllables[rand_choice(num_syllable)]);
	else
	    buf.strcat(syllables[syl]);
    }

    return (gram_capitalize(buf));
}

//
// Either looks up the artifact, or builds it from scratch and adds it
// to our cache.
// The artifact data structure tells the ITEM::get functions how to
// overload the items behaviour.
// Just started a nice glass of wine with those code.
// Be warned.  Music just stopped.  Must restart it.  New song is:
// Gypsy Kings: Bem, Bem, Maria
//
ARTIFACT *
artifact_buildartifact(const char *name, ITEM_NAMES baseitem)
{
    // Search for an existing artifact definition.
    ARTIFACT		*art;
    ITEMTYPE_NAMES	 itemtype;

    if (!name)
	return 0;

    for (art = glb_artifacts; art; art = art->next)
    {
	if (baseitem == art->baseitem && !strcmp(art->name, name))
	{
	    return art;
	}
    }

    // No artifact found.  Build a new one.
    art = new ARTIFACT();
    art->next = glb_artifacts;
    glb_artifacts = art;

    art->name = strdup(name);
    art->baseitem = baseitem;
    
    itemtype = (ITEMTYPE_NAMES) glb_itemdefs[baseitem].itemtype;

    // Now onto Guns & Roses: You're Crazy.
    INTRINSIC_NAMES		int_good[] =
    {
	INTRINSIC_FAST,
	INTRINSIC_QUICK,
	INTRINSIC_UNCHANGING,
	INTRINSIC_RESISTSTONING,
	INTRINSIC_RESISTFIRE,
	INTRINSIC_RESISTCOLD,
	INTRINSIC_RESISTSHOCK,
	INTRINSIC_RESISTACID,
	INTRINSIC_RESISTPOISON,
	INTRINSIC_RESISTPHYSICAL, // I think this is too powerful.
	                          // I think it's cool and very rare. --red_kangaroo
	INTRINSIC_TELEPORTCONTROL,
	INTRINSIC_POLYCONTROL,
	INTRINSIC_REGENERATION,
	INTRINSIC_REFLECTION,
	INTRINSIC_SEARCH,	// In some ways, this is bad :>
	INTRINSIC_INVISIBLE,
	INTRINSIC_SEEINVISIBLE,
	INTRINSIC_TELEPATHY,
	INTRINSIC_NOBREATH,
	INTRINSIC_LIFESAVE,	// You don't want this on +7 sword :>
	                    // Especially if it kicks in prior to
	                    // your amulet :>
	INTRINSIC_RESISTSLEEP,
	INTRINSIC_FREEDOM,
	INTRINSIC_WATERWALK,
	INTRINSIC_JUMP,
	INTRINSIC_WARNING,
	INTRINSIC_DIG,
	INTRINSIC_LICHFORM,
	INTRINSIC_NONE
    };

    INTRINSIC_NAMES		int_bad[] =
    {
	INTRINSIC_SLOW,
	INTRINSIC_TELEPORT,
	INTRINSIC_TELEFIXED,
	INTRINSIC_POSITIONREVEALED,
	INTRINSIC_MISSINGFINGER,
	INTRINSIC_MAGICDRAIN,
	INTRINSIC_BLEED,
	INTRINSIC_VULNSILVER,
	INTRINSIC_VULNFIRE,
	INTRINSIC_VULNCOLD,
	INTRINSIC_VULNACID,
	INTRINSIC_VULNSHOCK,
	INTRINSIC_VULNPHYSICAL,
	INTRINSIC_BLIND,
	INTRINSIC_STRANGLE,
	INTRINSIC_TIRED,
	INTRINSIC_AFLAME,	// Look forward to fireresist + aflame :>
	INTRINSIC_POLYMORPH,
	INTRINSIC_NOISY,
	INTRINSIC_DEAF,
	INTRINSIC_CONFUSED,
	INTRINSIC_AMNESIA,
	INTRINSIC_NOREGEN,
	INTRINSIC_LEFTHANDED,  // Does nothing good for you, so goes to bad ones.
	INTRINSIC_GOLDALLERGY,
	INTRINSIC_OFFBALANCE,
	INTRINSIC_NONE
    };
	
	INTRINSIC_NAMES		int_skill[] =
    {
    INTRINSIC_SKILL_ARMOUR_HELMET,
	INTRINSIC_SKILL_ARMOUR_SHIELD,
	INTRINSIC_SKILL_ARMOUR_BODY,
	INTRINSIC_SKILL_ARMOUR_BOOTS,
	INTRINSIC_SKILL_ARMOUR_CLOTH,
	INTRINSIC_SKILL_ARMOUR_LEATHER,
	INTRINSIC_SKILL_ARMOUR_IRON,
	INTRINSIC_SKILL_ARMOUR_EXOTIC,
	INTRINSIC_SKILL_DODGE,
	INTRINSIC_SKILL_MOVINGTARGET,
	INTRINSIC_SKILL_EVADETRAP,
	INTRINSIC_SKILL_TWOWEAPON,
	INTRINSIC_SKILL_WEAPON_RANGED,
	INTRINSIC_SKILL_WEAPON_IMPROVISE,
	INTRINSIC_SKILL_WEAPON_SMALL,
	INTRINSIC_SKILL_WEAPON_MEDIUM,
	INTRINSIC_SKILL_WEAPON_LARGE,
	INTRINSIC_SKILL_WEAPON_EDGED,
	INTRINSIC_SKILL_WEAPON_POINTED,
	INTRINSIC_SKILL_WEAPON_BLUNT,
	INTRINSIC_SKILL_WEAPON_TRUEAIM,
	INTRINSIC_SKILL_WEAPON_PARRY,
	INTRINSIC_SKILL_WEAPON_RIPOSTE,
	INTRINSIC_SKILL_WEAPON_KNOCKOUT,
	INTRINSIC_SKILL_WEAPON_KNOCKBACK,
	INTRINSIC_SKILL_WEAPON_STUN,
	INTRINSIC_SKILL_WEAPON_BLEEDINGWOUND,
	INTRINSIC_SKILL_WEAPON_DISARM,
	INTRINSIC_SKILL_WEAPON_IMPALE,
	INTRINSIC_SKILL_WEAPON_RICOCHET,
	INTRINSIC_SKILL_WEAPON_SNEAKATTACK,
	INTRINSIC_SKILL_ENDUREHUNGER,
	INTRINSIC_SKILL_BUTCHERY,
	INTRINSIC_SKILL_CLEANKILL,
	INTRINSIC_SKILL_CHARGE,
	INTRINSIC_SKILL_LEAPATTACK,
	INTRINSIC_NONE
    };
	
	INTRINSIC_NAMES		int_spell[] =
    {
	INTRINSIC_SPELL_FLASH,
	INTRINSIC_SPELL_STICKYFLAMES,
	INTRINSIC_SPELL_MAGICMISSILE,
	INTRINSIC_SPELL_CHILL,
	INTRINSIC_SPELL_SPARK,
	INTRINSIC_SPELL_FROSTBOLT,
	INTRINSIC_SPELL_LIVINGFROST, // No Blizzard, too hard to figure out how to make it work without prereq.
	INTRINSIC_SPELL_FIREBALL,
	INTRINSIC_SPELL_FLAMESTRIKE,
	INTRINSIC_SPELL_LIGHTNINGBOLT,
	INTRINSIC_SPELL_CHAINLIGHTNING,
	INTRINSIC_SPELL_SUNFIRE,
	INTRINSIC_SPELL_ACIDSPLASH,
	INTRINSIC_SPELL_ACIDICMIST,
	INTRINSIC_SPELL_CORROSIVEEXPLOSION,
	INTRINSIC_SPELL_ACIDPOOL,
	INTRINSIC_SPELL_MINDACID,
	INTRINSIC_SPELL_DISINTEGRATE,
	INTRINSIC_SPELL_REGENERATE,
	INTRINSIC_SPELL_SLOWPOISON,
	INTRINSIC_SPELL_HEAL,
	INTRINSIC_SPELL_CUREPOISON,
	INTRINSIC_SPELL_MAJORHEAL,
	INTRINSIC_SPELL_RESURRECT,
	INTRINSIC_SPELL_SUMMON_FAMILIAR,
	INTRINSIC_SPELL_TRANSFER_KNOWLEDGE,
	INTRINSIC_SPELL_LIGHT,
	INTRINSIC_SPELL_FORCEBOLT,
	INTRINSIC_SPELL_FORCEWALL,
	INTRINSIC_SPELL_DIG,
	INTRINSIC_SPELL_CREATEPIT,
	INTRINSIC_SPELL_SANDSTORM,
	INTRINSIC_SPELL_GROWFOREST,
	INTRINSIC_SPELL_ANIMATEFOREST,
	INTRINSIC_SPELL_DOWNPOUR,
	INTRINSIC_SPELL_ROLLINGBOULDER,
	INTRINSIC_SPELL_ENTOMB, // Now this is gonna be fun if someone forgets they don't need to have Dig...
	INTRINSIC_SPELL_KNOCK,
	INTRINSIC_SPELL_TRACK,
	INTRINSIC_SPELL_WIZARDSEYE,
	INTRINSIC_SPELL_DIAGNOSE,
	INTRINSIC_SPELL_POSSESS,
	INTRINSIC_SPELL_PRESERVE,
	INTRINSIC_SPELL_MAGICMAP,
	INTRINSIC_SPELL_TELEPORT,
	INTRINSIC_SPELL_BLINK,
	INTRINSIC_SPELL_FETCH,
	INTRINSIC_SPELL_IDENTIFY,
	INTRINSIC_SPELL_DETECTCURSE,
	INTRINSIC_SPELL_DIRECTWIND,
	INTRINSIC_SPELL_PETRIFY,
	INTRINSIC_SPELL_FINGEROFDEATH,
	INTRINSIC_SPELL_POISONITEM,
	INTRINSIC_SPELL_POISONBOLT,
	INTRINSIC_SPELL_RAISE_UNDEAD,
	INTRINSIC_SPELL_RECLAIM_SOUL,
	INTRINSIC_SPELL_DARK_RITUAL,
	INTRINSIC_SPELL_GHASTIFY,
	INTRINSIC_SPELL_BINDSOUL,
	INTRINSIC_SPELL_SOULSUCK,
	INTRINSIC_SPELL_SUMMON_IMP,
	INTRINSIC_SPELL_SUMMON_DEMON,
	INTRINSIC_SPELL_CLOUDKILL,
	INTRINSIC_NONE
    };

    int			numgood, numbad, numskill, numspell;

    // There is a cool sizeof() method that could simplify this.
    // Too lazy to figure it out, however.
    for (numgood = 0; int_good[numgood] != INTRINSIC_NONE; numgood++);
    for (numbad = 0; int_bad[numbad] != INTRINSIC_NONE; numbad++);
	for (numskill = 0; int_skill[numskill] != INTRINSIC_NONE; numskill++);
	for (numspell = 0; int_spell[numspell] != INTRINSIC_NONE; numspell++);

    // Ensure we start picking well
    artifact_seed(name);
    
	// There is a cumulative 10% chance of a bad intrinsic.
    // There is a 90% chance of good intrinsic, unless there is already a bad one, then it's 100%.
    // After that, we have a 10% chance per intrinsic.
    char		intbuf[100];
    int			numints;

    numints = 0;
    while (artifact_randchance(10) && numints < 99)
    {
	intbuf[numints++] = int_bad[artifact_randchoice(numbad)];
    }
    if (numints || artifact_randchance(90))
    {
	do
	{
	    intbuf[numints++] = int_good[artifact_randchoice(numgood)];
	} while (artifact_randchance(10) && numints < 99);
    }
	while (artifact_randchance(20) && numints < 99)
    {
	intbuf[numints++] = int_skill[artifact_randchoice(numskill)];
    }
	if ((itemtype == ITEMTYPE_WAND || artifact_randchance(10)) && numints < 99)
    {
	do
	{
	    intbuf[numints++] = int_spell[artifact_randchoice(numspell)];
	} while (artifact_randchance(10) && numints < 99);
    }

    // If we had any intrinsics, we want to add them to art.
    if (numints)
    {
	intbuf[numints++] = '\0';
	art->intrinsics = strdup(intbuf);
    }

    // Determine light radius.  Artifacts lacking intrinsics
    // will always get light.  Light is really cool!
    // NOTE: Because of this, a lit artifact has a 50% chance
    // of having no intrinsics!
    if (!numints || artifact_randchance(10))
    {
	art->lightradius = 2;
	// 30% for each higher amount.
	while (art->lightradius < 6 && artifact_randchance(30))
	{
	    art->lightradius++;
	}
    }

    // Determine if we get carry intrinsic.  This is *very* powerful.
    // Be careful about giving this.
    if (numints && artifact_randchance(1))
	art->iscarryintrinsic = 1;

    // Armour types get an AC bonus.
    // Weapons have a 10% chance of a bonus.  Extra bonuses
    // occur with a 30% chance.
    int		acbonus = 0;
    bool	weapac = artifact_randchance(10);
    do
    {
	acbonus++;
    } while (artifact_randchance(30) && (acbonus < 99));

    if (itemtype == ITEMTYPE_ARMOUR || itemtype == ITEMTYPE_AMULET || (itemtype == ITEMTYPE_WEAPON && weapac))
    {
	art->acbonus = acbonus;
    }

    // Determine bonus attacks.
    // Weapons always get a bonus attack.  Other things
    // a 10% chance.
    int			numverbs, verb;
    
    // Possible verb/element combinations
    const char *verbs[] =
    {
	"pulverize",
	"burn",
	"freeze",
	"zap",
	"dissolve",
	"blind",
	"drain",
	0
    };

    const ELEMENT_NAMES elements[] =
    {
	ELEMENT_PHYSICAL,
	ELEMENT_FIRE,
	ELEMENT_COLD,
	ELEMENT_SHOCK,
	ELEMENT_ACID,
	ELEMENT_LIGHT,
	ELEMENT_DEATH
    };

    for (numverbs = 0; verbs[numverbs]; numverbs++);

    art->hasattack = false;
    if (itemtype == ITEMTYPE_WEAPON || itemtype == ITEMTYPE_RING || artifact_randchance(10))
    {
	art->hasattack = true;

	verb = artifact_randchoice(numverbs);
	art->attack.verb = verbs[verb];

	// A random damage amount.
	art->attack.damage.myNumdie = artifact_randchoice(2) + 1;
	art->attack.damage.mySides = artifact_randchoice(8) + 1;
	art->attack.damage.myBonus = artifact_randchoice(4);

	// Inherit our base item's bonus to hit
	art->attack.bonustohit = glb_attackdefs[glb_itemdefs[baseitem].attack].bonustohit;
	// And get a bit extra...
	while (artifact_randchance(30) && (art->attack.bonustohit < 99))
	{
	    art->attack.bonustohit++;
	}

	art->attack.element = elements[verb];

	// Chain the attack.
	if (glb_itemdefs[baseitem].attack != ATTACK_MISUSED)
	    art->attack.sameattack = glb_itemdefs[baseitem].attack;
    }

    // Determine thrown attack.
    // Weapons have a 10% chance.  Thrown weapons 100% chance. Potions also have 100% chance.
    art->hasthrownattack = false;
    if ((itemtype == ITEMTYPE_WEAPON && (glb_itemdefs[baseitem].thrownattack != ATTACK_MISTHROWN || artifact_randchance(10))) ||
	     itemtype == ITEMTYPE_POTION)
    {
	art->hasthrownattack = true;

	// Determine the range...
	art->thrownattack.range = glb_attackdefs[glb_itemdefs[baseitem].thrownattack].range;
	while (artifact_randchance(30) && (art->thrownattack.range < 99))
	{
	    art->thrownattack.range++;
	}

	// Inherit our base item's bonus to hit
	art->thrownattack.bonustohit = glb_attackdefs[glb_itemdefs[baseitem].thrownattack].bonustohit;
	// And get a bit extra...
	while (artifact_randchance(30) && (art->thrownattack.bonustohit < 99))
	{
	    art->thrownattack.bonustohit++;
	}
	
	// A random damage amount.
	art->thrownattack.damage.myNumdie = artifact_randchoice(2) + 1;
	art->thrownattack.damage.mySides = artifact_randchoice(8) + 1;
	art->thrownattack.damage.myBonus = artifact_randchoice(4);

	// Chain the attack.
	if (glb_itemdefs[baseitem].thrownattack != ATTACK_MISTHROWN)
	    art->thrownattack.sameattack = glb_itemdefs[baseitem].thrownattack;
    }

    // TODO: Zappable artifacts!
	//       Bonus maxHP and maxMP.


    return art;
}
