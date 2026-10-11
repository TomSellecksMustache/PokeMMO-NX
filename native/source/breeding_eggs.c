#include "breeding.h"
#include <stdio.h>
#include <string.h>

// Egg groups of the species of generations 1 to 5, from the PokeAPI data (https://github.com/PokeAPI/pokeapi, CSV files), under their names in PokeMMO.
// In alphabetical order, as the species picker lists them. A group is a bit of the mask, in the order of GROUPS.
static const char *const GROUPS[] = {"Monster", "Water 1", "Bug", "Flying", "Field", "Fairy", "Grass", "Human-Like", "Water 3", "Mineral", "Amorphous", "Water 2", "Ditto", "Dragon", "Undiscovered"};
static const struct {
    const char *name;
    unsigned short groups;
} SPECIES[] = {
    {"Abomasnow", 0x0041}, {"Abra", 0x0080}, {"Absol", 0x0010}, {"Accelgor", 0x0004}, {"Aerodactyl", 0x0008}, {"Aggron", 0x0001}, {"Aipom", 0x0010},
    {"Alakazam", 0x0080}, {"Alomomola", 0x0802}, {"Altaria", 0x2008}, {"Ambipom", 0x0010}, {"Amoonguss", 0x0040}, {"Ampharos", 0x0011},
    {"Anorith", 0x0100}, {"Arbok", 0x2010}, {"Arcanine", 0x0010}, {"Arceus", 0x4000}, {"Archen", 0x0108}, {"Archeops", 0x0108}, {"Ariados", 0x0004},
    {"Armaldo", 0x0100}, {"Aron", 0x0001}, {"Articuno", 0x4000}, {"Audino", 0x0020}, {"Axew", 0x2001}, {"Azelf", 0x4000}, {"Azumarill", 0x0022},
    {"Azurill", 0x4000}, {"Bagon", 0x2000}, {"Baltoy", 0x0200}, {"Banette", 0x0400}, {"Barboach", 0x0800}, {"Basculin", 0x0800},
    {"Bastiodon", 0x0001}, {"Bayleef", 0x0041}, {"Beartic", 0x0010}, {"Beautifly", 0x0004}, {"Beedrill", 0x0004}, {"Beheeyem", 0x0080},
    {"Beldum", 0x0200}, {"Bellossom", 0x0040}, {"Bellsprout", 0x0040}, {"Bibarel", 0x0012}, {"Bidoof", 0x0012}, {"Bisharp", 0x0080},
    {"Blastoise", 0x0003}, {"Blaziken", 0x0010}, {"Blissey", 0x0020}, {"Blitzle", 0x0010}, {"Boldore", 0x0200}, {"Bonsly", 0x4000},
    {"Bouffalant", 0x0010}, {"Braviary", 0x0008}, {"Breloom", 0x0060}, {"Bronzong", 0x0200}, {"Bronzor", 0x0200}, {"Budew", 0x4000},
    {"Buizel", 0x0012}, {"Bulbasaur", 0x0041}, {"Buneary", 0x0090}, {"Burmy", 0x0004}, {"Butterfree", 0x0004}, {"Cacnea", 0x00c0},
    {"Cacturne", 0x00c0}, {"Camerupt", 0x0010}, {"Carnivine", 0x0040}, {"Carracosta", 0x0102}, {"Carvanha", 0x0800}, {"Cascoon", 0x0004},
    {"Castform", 0x0420}, {"Caterpie", 0x0004}, {"Celebi", 0x4000}, {"Chandelure", 0x0400}, {"Chansey", 0x0020}, {"Charizard", 0x2001},
    {"Charmander", 0x2001}, {"Charmeleon", 0x2001}, {"Chatot", 0x0008}, {"Cherrim", 0x0060}, {"Cherubi", 0x0060}, {"Chikorita", 0x0041},
    {"Chimchar", 0x0090}, {"Chimecho", 0x0400}, {"Chinchou", 0x0800}, {"Chingling", 0x4000}, {"Cinccino", 0x0010}, {"Clamperl", 0x0002},
    {"Claydol", 0x0200}, {"Clefable", 0x0020}, {"Clefairy", 0x0020}, {"Cleffa", 0x4000}, {"Cloyster", 0x0100}, {"Cobalion", 0x4000},
    {"Cofagrigus", 0x0600}, {"Combee", 0x0004}, {"Combusken", 0x0010}, {"Conkeldurr", 0x0080}, {"Corphish", 0x0102}, {"Corsola", 0x0102},
    {"Cottonee", 0x0060}, {"Cradily", 0x0100}, {"Cranidos", 0x0001}, {"Crawdaunt", 0x0102}, {"Cresselia", 0x4000}, {"Croagunk", 0x0080},
    {"Crobat", 0x0008}, {"Croconaw", 0x0003}, {"Crustle", 0x0204}, {"Cryogonal", 0x0200}, {"Cubchoo", 0x0010}, {"Cubone", 0x0001},
    {"Cyndaquil", 0x0010}, {"Darkrai", 0x4000}, {"Darmanitan", 0x0010}, {"Darumaka", 0x0010}, {"Deerling", 0x0010}, {"Deino", 0x2000},
    {"Delcatty", 0x0030}, {"Delibird", 0x0012}, {"Deoxys", 0x4000}, {"Dewgong", 0x0012}, {"Dewott", 0x0010}, {"Dialga", 0x4000}, {"Diglett", 0x0010},
    {"Ditto", 0x1000}, {"Dodrio", 0x0008}, {"Doduo", 0x0008}, {"Donphan", 0x0010}, {"Dragonair", 0x2002}, {"Dragonite", 0x2002}, {"Drapion", 0x0104},
    {"Dratini", 0x2002}, {"Drifblim", 0x0400}, {"Drifloon", 0x0400}, {"Drilbur", 0x0010}, {"Drowzee", 0x0080}, {"Druddigon", 0x2001},
    {"Ducklett", 0x000a}, {"Dugtrio", 0x0010}, {"Dunsparce", 0x0010}, {"Duosion", 0x0400}, {"Durant", 0x0004}, {"Dusclops", 0x0400},
    {"Dusknoir", 0x0400}, {"Duskull", 0x0400}, {"Dustox", 0x0004}, {"Dwebble", 0x0204}, {"Eelektrik", 0x0400}, {"Eelektross", 0x0400},
    {"Eevee", 0x0010}, {"Ekans", 0x2010}, {"Electabuzz", 0x0080}, {"Electivire", 0x0080}, {"Electrike", 0x0010}, {"Electrode", 0x0200},
    {"Elekid", 0x4000}, {"Elgyem", 0x0080}, {"Emboar", 0x0010}, {"Emolga", 0x0010}, {"Empoleon", 0x0012}, {"Entei", 0x4000}, {"Escavalier", 0x0004},
    {"Espeon", 0x0010}, {"Excadrill", 0x0010}, {"Exeggcute", 0x0040}, {"Exeggutor", 0x0040}, {"Exploud", 0x0011}, {"Farfetch'd", 0x0018},
    {"Fearow", 0x0008}, {"Feebas", 0x2002}, {"Feraligatr", 0x0003}, {"Ferroseed", 0x0240}, {"Ferrothorn", 0x0240}, {"Finneon", 0x0800},
    {"Flaaffy", 0x0011}, {"Flareon", 0x0010}, {"Floatzel", 0x0012}, {"Flygon", 0x2004}, {"Foongus", 0x0040}, {"Forretress", 0x0004},
    {"Fraxure", 0x2001}, {"Frillish", 0x0400}, {"Froslass", 0x0220}, {"Furret", 0x0010}, {"Gabite", 0x2001}, {"Gallade", 0x0480},
    {"Galvantula", 0x0004}, {"Garbodor", 0x0200}, {"Garchomp", 0x2001}, {"Gardevoir", 0x0480}, {"Gastly", 0x0400}, {"Gastrodon", 0x0402},
    {"Genesect", 0x4000}, {"Gengar", 0x0400}, {"Geodude", 0x0200}, {"Gible", 0x2001}, {"Gigalith", 0x0200}, {"Girafarig", 0x0010},
    {"Giratina", 0x4000}, {"Glaceon", 0x0010}, {"Glalie", 0x0220}, {"Glameow", 0x0010}, {"Gligar", 0x0004}, {"Gliscor", 0x0004}, {"Gloom", 0x0040},
    {"Golbat", 0x0008}, {"Goldeen", 0x0800}, {"Golduck", 0x0012}, {"Golem", 0x0200}, {"Golett", 0x0200}, {"Golurk", 0x0200}, {"Gorebyss", 0x0002},
    {"Gothita", 0x0080}, {"Gothitelle", 0x0080}, {"Gothorita", 0x0080}, {"Granbull", 0x0030}, {"Graveler", 0x0200}, {"Grimer", 0x0400},
    {"Grotle", 0x0041}, {"Groudon", 0x4000}, {"Grovyle", 0x2001}, {"Growlithe", 0x0010}, {"Grumpig", 0x0010}, {"Gulpin", 0x0400}, {"Gurdurr", 0x0080},
    {"Gyarados", 0x2800}, {"Happiny", 0x4000}, {"Hariyama", 0x0080}, {"Haunter", 0x0400}, {"Haxorus", 0x2001}, {"Heatmor", 0x0010},
    {"Heatran", 0x4000}, {"Heracross", 0x0004}, {"Herdier", 0x0010}, {"Hippopotas", 0x0010}, {"Hippowdon", 0x0010}, {"Hitmonchan", 0x0080},
    {"Hitmonlee", 0x0080}, {"Hitmontop", 0x0080}, {"Ho-Oh", 0x4000}, {"Honchkrow", 0x0008}, {"Hoothoot", 0x0008}, {"Hoppip", 0x0060},
    {"Horsea", 0x2002}, {"Houndoom", 0x0010}, {"Houndour", 0x0010}, {"Huntail", 0x0002}, {"Hydreigon", 0x2000}, {"Hypno", 0x0080},
    {"Igglybuff", 0x4000}, {"Illumise", 0x0084}, {"Infernape", 0x0090}, {"Ivysaur", 0x0041}, {"Jellicent", 0x0400}, {"Jigglypuff", 0x0020},
    {"Jirachi", 0x4000}, {"Jolteon", 0x0010}, {"Joltik", 0x0004}, {"Jumpluff", 0x0060}, {"Jynx", 0x0080}, {"Kabuto", 0x0102}, {"Kabutops", 0x0102},
    {"Kadabra", 0x0080}, {"Kakuna", 0x0004}, {"Kangaskhan", 0x0001}, {"Karrablast", 0x0004}, {"Kecleon", 0x0010}, {"Keldeo", 0x4000},
    {"Kingdra", 0x2002}, {"Kingler", 0x0100}, {"Kirlia", 0x0480}, {"Klang", 0x0200}, {"Klink", 0x0200}, {"Klinklang", 0x0200}, {"Koffing", 0x0400},
    {"Krabby", 0x0100}, {"Kricketot", 0x0004}, {"Kricketune", 0x0004}, {"Krokorok", 0x0010}, {"Krookodile", 0x0010}, {"Kyogre", 0x4000},
    {"Kyurem", 0x4000}, {"Lairon", 0x0001}, {"Lampent", 0x0400}, {"Landorus", 0x4000}, {"Lanturn", 0x0800}, {"Lapras", 0x0003}, {"Larvesta", 0x0004},
    {"Larvitar", 0x0001}, {"Latias", 0x4000}, {"Latios", 0x4000}, {"Leafeon", 0x0010}, {"Leavanny", 0x0004}, {"Ledian", 0x0004}, {"Ledyba", 0x0004},
    {"Lickilicky", 0x0001}, {"Lickitung", 0x0001}, {"Liepard", 0x0010}, {"Lileep", 0x0100}, {"Lilligant", 0x0040}, {"Lillipup", 0x0010},
    {"Linoone", 0x0010}, {"Litwick", 0x0400}, {"Lombre", 0x0042}, {"Lopunny", 0x0090}, {"Lotad", 0x0042}, {"Loudred", 0x0011}, {"Lucario", 0x0090},
    {"Ludicolo", 0x0042}, {"Lugia", 0x4000}, {"Lumineon", 0x0800}, {"Lunatone", 0x0200}, {"Luvdisc", 0x0800}, {"Luxio", 0x0010}, {"Luxray", 0x0010},
    {"Machamp", 0x0080}, {"Machoke", 0x0080}, {"Machop", 0x0080}, {"Magby", 0x4000}, {"Magcargo", 0x0400}, {"Magikarp", 0x2800}, {"Magmar", 0x0080},
    {"Magmortar", 0x0080}, {"Magnemite", 0x0200}, {"Magneton", 0x0200}, {"Magnezone", 0x0200}, {"Makuhita", 0x0080}, {"Mamoswine", 0x0010},
    {"Manaphy", 0x0022}, {"Mandibuzz", 0x0008}, {"Manectric", 0x0010}, {"Mankey", 0x0010}, {"Mantine", 0x0002}, {"Mantyke", 0x4000},
    {"Maractus", 0x0040}, {"Mareep", 0x0011}, {"Marill", 0x0022}, {"Marowak", 0x0001}, {"Marshtomp", 0x0003}, {"Masquerain", 0x0006},
    {"Mawile", 0x0030}, {"Medicham", 0x0080}, {"Meditite", 0x0080}, {"Meganium", 0x0041}, {"Meloetta", 0x4000}, {"Meowth", 0x0010},
    {"Mesprit", 0x4000}, {"Metagross", 0x0200}, {"Metang", 0x0200}, {"Metapod", 0x0004}, {"Mew", 0x4000}, {"Mewtwo", 0x4000}, {"Mienfoo", 0x0090},
    {"Mienshao", 0x0090}, {"Mightyena", 0x0010}, {"Milotic", 0x2002}, {"Miltank", 0x0010}, {"Mime Jr.", 0x4000}, {"Minccino", 0x0010},
    {"Minun", 0x0020}, {"Misdreavus", 0x0400}, {"Mismagius", 0x0400}, {"Moltres", 0x4000}, {"Monferno", 0x0090}, {"Mothim", 0x0004},
    {"Mr. Mime", 0x0080}, {"Mudkip", 0x0003}, {"Muk", 0x0400}, {"Munchlax", 0x4000}, {"Munna", 0x0010}, {"Murkrow", 0x0008}, {"Musharna", 0x0010},
    {"Natu", 0x0008}, {"Nidoking", 0x0011}, {"Nidoqueen", 0x4000}, {"Nidoran F", 0x0011}, {"Nidoran M", 0x0011}, {"Nidorina", 0x4000},
    {"Nidorino", 0x0011}, {"Nincada", 0x0004}, {"Ninetales", 0x0010}, {"Ninjask", 0x0004}, {"Noctowl", 0x0008}, {"Nosepass", 0x0200},
    {"Numel", 0x0010}, {"Nuzleaf", 0x0050}, {"Octillery", 0x0802}, {"Oddish", 0x0040}, {"Omanyte", 0x0102}, {"Omastar", 0x0102}, {"Onix", 0x0200},
    {"Oshawott", 0x0010}, {"Pachirisu", 0x0030}, {"Palkia", 0x4000}, {"Palpitoad", 0x0002}, {"Panpour", 0x0010}, {"Pansage", 0x0010},
    {"Pansear", 0x0010}, {"Paras", 0x0044}, {"Parasect", 0x0044}, {"Patrat", 0x0010}, {"Pawniard", 0x0080}, {"Pelipper", 0x000a}, {"Persian", 0x0010},
    {"Petilil", 0x0040}, {"Phanpy", 0x0010}, {"Phione", 0x0022}, {"Pichu", 0x4000}, {"Pidgeot", 0x0008}, {"Pidgeotto", 0x0008}, {"Pidgey", 0x0008},
    {"Pidove", 0x0008}, {"Pignite", 0x0010}, {"Pikachu", 0x0030}, {"Piloswine", 0x0010}, {"Pineco", 0x0004}, {"Pinsir", 0x0004}, {"Piplup", 0x0012},
    {"Plusle", 0x0020}, {"Politoed", 0x0002}, {"Poliwag", 0x0002}, {"Poliwhirl", 0x0002}, {"Poliwrath", 0x0002}, {"Ponyta", 0x0010},
    {"Poochyena", 0x0010}, {"Porygon", 0x0200}, {"Porygon-Z", 0x0200}, {"Porygon2", 0x0200}, {"Primeape", 0x0010}, {"Prinplup", 0x0012},
    {"Probopass", 0x0200}, {"Psyduck", 0x0012}, {"Pupitar", 0x0001}, {"Purrloin", 0x0010}, {"Purugly", 0x0010}, {"Quagsire", 0x0012},
    {"Quilava", 0x0010}, {"Qwilfish", 0x0800}, {"Raichu", 0x0030}, {"Raikou", 0x4000}, {"Ralts", 0x0480}, {"Rampardos", 0x0001}, {"Rapidash", 0x0010},
    {"Raticate", 0x0010}, {"Rattata", 0x0010}, {"Rayquaza", 0x4000}, {"Regice", 0x4000}, {"Regigigas", 0x4000}, {"Regirock", 0x4000},
    {"Registeel", 0x4000}, {"Relicanth", 0x0802}, {"Remoraid", 0x0802}, {"Reshiram", 0x4000}, {"Reuniclus", 0x0400}, {"Rhydon", 0x0011},
    {"Rhyhorn", 0x0011}, {"Rhyperior", 0x0011}, {"Riolu", 0x4000}, {"Roggenrola", 0x0200}, {"Roselia", 0x0060}, {"Roserade", 0x0060},
    {"Rotom", 0x0400}, {"Rufflet", 0x0008}, {"Sableye", 0x0080}, {"Salamence", 0x2000}, {"Samurott", 0x0010}, {"Sandile", 0x0010},
    {"Sandshrew", 0x0010}, {"Sandslash", 0x0010}, {"Sawk", 0x0080}, {"Sawsbuck", 0x0010}, {"Sceptile", 0x2001}, {"Scizor", 0x0004},
    {"Scolipede", 0x0004}, {"Scrafty", 0x2010}, {"Scraggy", 0x2010}, {"Scyther", 0x0004}, {"Seadra", 0x2002}, {"Seaking", 0x0800}, {"Sealeo", 0x0012},
    {"Seedot", 0x0050}, {"Seel", 0x0012}, {"Seismitoad", 0x0002}, {"Sentret", 0x0010}, {"Serperior", 0x0050}, {"Servine", 0x0050},
    {"Seviper", 0x2010}, {"Sewaddle", 0x0004}, {"Sharpedo", 0x0800}, {"Shaymin", 0x4000}, {"Shedinja", 0x0200}, {"Shelgon", 0x2000},
    {"Shellder", 0x0100}, {"Shellos", 0x0402}, {"Shelmet", 0x0004}, {"Shieldon", 0x0001}, {"Shiftry", 0x0050}, {"Shinx", 0x0010},
    {"Shroomish", 0x0060}, {"Shuckle", 0x0004}, {"Shuppet", 0x0400}, {"Sigilyph", 0x0008}, {"Silcoon", 0x0004}, {"Simipour", 0x0010},
    {"Simisage", 0x0010}, {"Simisear", 0x0010}, {"Skarmory", 0x0008}, {"Skiploom", 0x0060}, {"Skitty", 0x0030}, {"Skorupi", 0x0104},
    {"Skuntank", 0x0010}, {"Slaking", 0x0010}, {"Slakoth", 0x0010}, {"Slowbro", 0x0003}, {"Slowking", 0x0003}, {"Slowpoke", 0x0003},
    {"Slugma", 0x0400}, {"Smeargle", 0x0010}, {"Smoochum", 0x4000}, {"Sneasel", 0x0010}, {"Snivy", 0x0050}, {"Snorlax", 0x0001}, {"Snorunt", 0x0220},
    {"Snover", 0x0041}, {"Snubbull", 0x0030}, {"Solosis", 0x0400}, {"Solrock", 0x0200}, {"Spearow", 0x0008}, {"Spheal", 0x0012}, {"Spinarak", 0x0004},
    {"Spinda", 0x0090}, {"Spiritomb", 0x0400}, {"Spoink", 0x0010}, {"Squirtle", 0x0003}, {"Stantler", 0x0010}, {"Staraptor", 0x0008},
    {"Staravia", 0x0008}, {"Starly", 0x0008}, {"Starmie", 0x0100}, {"Staryu", 0x0100}, {"Steelix", 0x0200}, {"Stoutland", 0x0010},
    {"Stunfisk", 0x0402}, {"Stunky", 0x0010}, {"Sudowoodo", 0x0200}, {"Suicune", 0x4000}, {"Sunflora", 0x0040}, {"Sunkern", 0x0040},
    {"Surskit", 0x0006}, {"Swablu", 0x2008}, {"Swadloon", 0x0004}, {"Swalot", 0x0400}, {"Swampert", 0x0003}, {"Swanna", 0x000a}, {"Swellow", 0x0008},
    {"Swinub", 0x0010}, {"Swoobat", 0x0018}, {"Taillow", 0x0008}, {"Tangela", 0x0040}, {"Tangrowth", 0x0040}, {"Tauros", 0x0010},
    {"Teddiursa", 0x0010}, {"Tentacool", 0x0100}, {"Tentacruel", 0x0100}, {"Tepig", 0x0010}, {"Terrakion", 0x4000}, {"Throh", 0x0080},
    {"Thundurus", 0x4000}, {"Timburr", 0x0080}, {"Tirtouga", 0x0102}, {"Togekiss", 0x0028}, {"Togepi", 0x4000}, {"Togetic", 0x0028},
    {"Torchic", 0x0010}, {"Torkoal", 0x0010}, {"Tornadus", 0x4000}, {"Torterra", 0x0041}, {"Totodile", 0x0003}, {"Toxicroak", 0x0080},
    {"Tranquill", 0x0008}, {"Trapinch", 0x2004}, {"Treecko", 0x2001}, {"Tropius", 0x0041}, {"Trubbish", 0x0200}, {"Turtwig", 0x0041},
    {"Tympole", 0x0002}, {"Tynamo", 0x0400}, {"Typhlosion", 0x0010}, {"Tyranitar", 0x0001}, {"Tyrogue", 0x4000}, {"Umbreon", 0x0010},
    {"Unfezant", 0x0008}, {"Unown", 0x4000}, {"Ursaring", 0x0010}, {"Uxie", 0x4000}, {"Vanillish", 0x0200}, {"Vanillite", 0x0200},
    {"Vanilluxe", 0x0200}, {"Vaporeon", 0x0010}, {"Venipede", 0x0004}, {"Venomoth", 0x0004}, {"Venonat", 0x0004}, {"Venusaur", 0x0041},
    {"Vespiquen", 0x0004}, {"Vibrava", 0x2004}, {"Victini", 0x4000}, {"Victreebel", 0x0040}, {"Vigoroth", 0x0010}, {"Vileplume", 0x0040},
    {"Virizion", 0x4000}, {"Volbeat", 0x0084}, {"Volcarona", 0x0004}, {"Voltorb", 0x0200}, {"Vullaby", 0x0008}, {"Vulpix", 0x0010},
    {"Wailmer", 0x0810}, {"Wailord", 0x0810}, {"Walrein", 0x0012}, {"Wartortle", 0x0003}, {"Watchog", 0x0010}, {"Weavile", 0x0010},
    {"Weedle", 0x0004}, {"Weepinbell", 0x0040}, {"Weezing", 0x0400}, {"Whimsicott", 0x0060}, {"Whirlipede", 0x0004}, {"Whiscash", 0x0800},
    {"Whismur", 0x0011}, {"Wigglytuff", 0x0020}, {"Wingull", 0x000a}, {"Wobbuffet", 0x0400}, {"Woobat", 0x0018}, {"Wooper", 0x0012},
    {"Wormadam", 0x0004}, {"Wurmple", 0x0004}, {"Wynaut", 0x4000}, {"Xatu", 0x0008}, {"Yamask", 0x0600}, {"Yanma", 0x0004}, {"Yanmega", 0x0004},
    {"Zangoose", 0x0010}, {"Zapdos", 0x4000}, {"Zebstrika", 0x0010}, {"Zekrom", 0x4000}, {"Zigzagoon", 0x0010}, {"Zoroark", 0x0010},
    {"Zorua", 0x0010}, {"Zubat", 0x0008}, {"Zweilous", 0x2000},
};
#define COUNT (sizeof(SPECIES) / sizeof(SPECIES[0]))
unsigned breedSpeciesCount(void) { return (unsigned)COUNT; }
const char *breedSpeciesName(unsigned index) { return index < COUNT ? SPECIES[index].name : ""; }
static bool groupsText(unsigned mask, char *out, size_t size) {
    size_t at = 0;
    if (size) out[0] = 0;
    for (unsigned g = 0; g < sizeof(GROUPS) / sizeof(GROUPS[0]); ++g)
        if ((mask >> g) & 1u) at += (size_t)snprintf(out + at, at < size ? size - at : 0, "%s%s", at ? " / " : "", GROUPS[g]);
    return at > 0;
}
bool breedSpeciesGroups(unsigned index, char *out, size_t size) { return index < COUNT && groupsText(SPECIES[index].groups, out, size); }
static void simplify(const char *name, char *key, size_t size) {  // lower case, letters and digits only: "Mr. Mime" is "mrmime"
    size_t length = 0;
    for (; *name && length + 1 < size; ++name) {
        char c = *name >= 'A' && *name <= 'Z' ? (char)(*name + 32) : *name;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) key[length++] = c;
    }
    key[length] = 0;
}
bool breedSpeciesStarts(unsigned index, const char *typed) {
    char wanted[32], key[32];
    simplify(typed, wanted, sizeof(wanted));
    simplify(index < COUNT ? SPECIES[index].name : "", key, sizeof(key));
    return strncmp(key, wanted, strlen(wanted)) == 0;
}
bool breedEggGroups(const char *species, char *out, size_t size) {
    char wanted[32], key[32];
    simplify(species, wanted, sizeof(wanted));
    for (unsigned i = 0; i < COUNT; ++i) {
        simplify(SPECIES[i].name, key, sizeof(key));
        if (strcmp(key, wanted) == 0) return groupsText(SPECIES[i].groups, out, size);
    }
    return false;
}
