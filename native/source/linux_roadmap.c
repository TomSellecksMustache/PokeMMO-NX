#include "linux_roadmap.h"
#include "breeding.h"
#include "linux_sdl_input.h"
#include "nxpanded_text.h"
#include "linux_visual_effects.h"
#include "linux_visual_menu.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define FILE_PATH "sdmc:/switch/PokeMMO-NXpanded/breeding.csv"  // the chain being worked on
#define CHAIN_DIR "sdmc:/switch/PokeMMO-NXpanded/chains"         // the ones put aside, a file each
#define CHAIN_PATH CHAIN_DIR "/c%02d.csv"
#define CHAINS 32
#define ROW 30.0f
#define TEXT_X 56.0f
#define COLUMNS 55u  // what fits in a panel at the font's size
#define LIST_ROWS 14
#define TOAST_DURATION_NS 10000000000ull
// SDL gamepad buttons
#define BTN_A (1u << 0)
#define BTN_B (1u << 1)
#define BTN_X (1u << 2)
#define BTN_Y (1u << 3)
#define BTN_MINUS (1u << 4)
#define BTN_PLUS (1u << 6)
#define BTN_L (1u << 9)
#define BTN_R (1u << 10)
#define BTN_UP (1u << 11)
#define BTN_DOWN (1u << 12)
#define BTN_LEFT (1u << 13)
#define BTN_RIGHT (1u << 14)
typedef enum { VIEW_NEXT, VIEW_CONFIRM, VIEW_STEPS, VIEW_ITEMS, VIEW_SETUP, VIEW_STOCK, VIEW_EDIT, VIEW_PICK, VIEW_CHAINS, VIEW_SHOPPING, VIEW_VISUAL } View;
typedef enum { ASK_DONE, ASK_REMOVE, ASK_CHAIN } Ask;
typedef struct {
    int slot;
    char label[72];
} Chain;

static BreedState state;
static BreedPlan plan;
static const char *problem;
static char eggs[64];
static View view, form, items_back, chains_back;  // form: the setup or edit view that the species picker returns to
static Ask ask;
static bool yes, pick_none, typing, quiet;  // typing: the console's keyboard is up for the species; quiet: it just left, the buttons it was given back with do not count
static int scroll, row, item_row, stock_row, edit, pick, chain_row, chain_count, visual_row, visual_tab;
static const char *visual_notice;
static Chain chains[CHAINS];
static const char *notice;  // why the last thing in the chains list did not happen
static char filter[24];  // what was typed of the species
static int match[700], matches;  // the species that begin with it: indexes, -1 for "none"
static uint64_t typing_since, quiet_until;
static atomic_bool active, hold, fresh;
static atomic_ullong toast_since; // 0 when dismissed; never captures game input
static atomic_flag guard = ATOMIC_FLAG_INIT;
static void lock(void) {
    while (atomic_flag_test_and_set_explicit(&guard, memory_order_acquire)) {}
}
static void unlock(void) { atomic_flag_clear_explicit(&guard, memory_order_release); }

static void refresh(void) {  // lock held
    problem = breedProblem(&state);
    plan.count = 0;
    if (!problem) breedPlan(&state, &plan);
    if (!breedEggGroups(state.species, eggs, sizeof(eggs))) snprintf(eggs, sizeof(eggs), "Unknown");
}
static void finish(void) {  // back to the next step, with everything that was changed saved and planned again
    breedSave(FILE_PATH, &state);
    refresh();
    view = VIEW_NEXT;
}
// ---- chains: the one being worked on is breeding.csv; the others wait in files of their own, each with its stock. The items are the same everywhere.
static void describe(const BreedState *chain, int left, char *out, size_t size) {  // "Gible 3x31 (HP/Atk/Spe) + Adamant, 12 left"
    if (!chain->species[0]) {
        snprintf(out, size, "(no target yet)");
        return;
    }
    BreedStep step = {.natured = chain->nature >= 0};
    char result[96];
    for (unsigned i = 0; i < BREED_STATS; ++i)
        if ((chain->ivs >> i) & 1u) step.order[step.count++] = (unsigned char)i;
    breedResult(chain, &step, result, sizeof(result));
    snprintf(out, size, "%.23s %.40s", chain->species, result);
    if (left >= 0) snprintf(out + strlen(out), size - strlen(out), ", %d left", left);
}
static void listChains(void) {
    static BreedState other;
    chain_count = 0;
    for (int slot = 1; slot <= CHAINS; ++slot) {
        char path[96];
        snprintf(path, sizeof(path), CHAIN_PATH, slot);
        if (!breedLoad(path, &other)) continue;
        chains[chain_count].slot = slot;
        describe(&other, other.left, chains[chain_count].label, sizeof(chains[0].label));
        chain_count++;
    }
}
// The chain being worked on goes to a file of its own: the slot is returned (0: there was nothing to keep, -1: it did not work).
// Without a free slot the file of `fallback` is taken over, when the caller has that chain in memory.
static int park(int fallback) {
    if (!state.species[0] && !state.ivs && !state.stock_count) return 0;
    int slot = 1;
    char path[96];
    for (; slot <= CHAINS; ++slot) {
        FILE *taken;
        snprintf(path, sizeof(path), CHAIN_PATH, slot);
        if (!(taken = fopen(path, "rb"))) break;
        fclose(taken);
    }
    if (slot > CHAINS && fallback) {
        slot = fallback;
        snprintf(path, sizeof(path), CHAIN_PATH, slot);
    }
    if (slot > CHAINS) {
        notice = "No room: delete a saved chain first.";
        return -1;
    }
    mkdir(CHAIN_DIR, 0777);
    state.left = problem ? -1 : (int)plan.count;
    bool saved = breedSave(path, &state);
    state.left = -1;
    if (!saved) notice = "Could not save the chain.";
    return saved ? slot : -1;
}
static void newChain(void) {  // keeps the items, starts without a target or stock
    if (park(0) < 0) return;
    state.species[0] = 0;
    state.nature = -1;
    state.ivs = state.stock_count = 0;
    finish();
    view = form = VIEW_SETUP;
    row = 0;
}
static void switchTo(int index) {  // the one being worked on is put aside first, so nothing is lost
    static BreedState other;
    char path[96];
    snprintf(path, sizeof(path), CHAIN_PATH, chains[index].slot);
    if (!breedLoad(path, &other)) {
        notice = "Could not read that chain.";
        return;
    }
    int slot = park(chains[index].slot);
    if (slot < 0) return;
    if (slot != chains[index].slot) remove(path);
    memcpy(other.have, state.have, sizeof(other.have));
    other.left = -1;
    state = other;
    finish();
}
static void openChains(View back) {
    chains_back = back;
    chain_row = 0;
    notice = NULL;
    listChains();
    view = VIEW_CHAINS;
}
static void show(View initial) {
    lock();
    atomic_store(&toast_since, 0); // full-screen view takes precedence over a pending toast
    memset(&state, 0, sizeof(state));
    state.nature = state.left = -1;
    if (!breedLoad(FILE_PATH, &state)) breedLoad(FILE_PATH ".tmp", &state);  // the temporary file is what is left if the console went off while saving
    refresh();
    view = initial == VIEW_SHOPPING || initial == VIEW_VISUAL ? initial : (problem ? VIEW_SETUP : VIEW_NEXT);  // an unset goal may still show its empty shopping list
    form = VIEW_SETUP;
    scroll = row = item_row = visual_row = visual_tab = 0;
    visual_notice = NULL;
    atomic_store(&fresh, true);
    atomic_store(&hold, true);
    atomic_store(&active, true);
    unlock();
}
void linuxRoadmapShow(void) { show(VIEW_NEXT); }
void linuxRoadmapShowShopping(void) { show(VIEW_SHOPPING); }
void linuxRoadmapShowVisual(void) { show(VIEW_VISUAL); }
void linuxRoadmapToastShow(uint64_t now_ns) {
    // No game memory or network access: the saved, manually entered breeding.csv is the sole input.
    // Called only while the modal roadmap and file picker are inactive.
    lock();
    memset(&state, 0, sizeof(state));
    state.nature = state.left = -1;
    if (!breedLoad(FILE_PATH, &state)) breedLoad(FILE_PATH ".tmp", &state);
    refresh();
    atomic_store(&toast_since, now_ns ? now_ns : 1);
    unlock();
}
float linuxRoadmapToastOpacity(uint64_t now_ns) {
    uint64_t since = atomic_load(&toast_since);
    if (!since || now_ns < since || now_ns - since >= TOAST_DURATION_NS) return 0.0f;
    return 1.0f - (float)(now_ns - since) / (float)TOAST_DURATION_NS;
}
bool linuxRoadmapOpen(void) { return atomic_load(&active); }
bool linuxRoadmapCapturesInput(void) { return atomic_load(&active) || atomic_load(&hold); }

// What the setup and the edit view change: the target, or one of the Pokemon the player owns.
static char *formName(size_t *size) {
    *size = form == VIEW_SETUP ? sizeof(state.species) : sizeof(state.stock[0].name);
    return form == VIEW_SETUP ? state.species : state.stock[edit].name;
}
static int *formNature(void) { return form == VIEW_SETUP ? &state.nature : &state.stock[edit].nature; }
static unsigned *formIvs(void) { return form == VIEW_SETUP ? &state.ivs : &state.stock[edit].ivs; }
static int formRows(void) { return form == VIEW_SETUP ? 11 : 9; }
static void rebuild(void) {  // the species that begin with what was typed
    matches = 0;
    if (pick_none && !filter[0]) match[matches++] = -1;
    for (unsigned i = 0; i < breedSpeciesCount(); ++i)
        if (breedSpeciesStarts(i, filter)) match[matches++] = (int)i;
    pick = pick > matches - 1 ? matches - 1 : pick;
    pick = pick < 0 ? 0 : pick;
}
static void choose(void) {  // lock held
    size_t size;
    char *name = formName(&size);
    if (matches) snprintf(name, size, "%s", match[pick] < 0 ? "Pokemon" : breedSpeciesName((unsigned)match[pick]));
    view = form;
}
// What the console's keyboard types goes here (not to the game) while the species is asked for: the list gets shorter with every letter.
static void typed(const char *text, int done) {
    lock();
    if (text) {
        while (*text == ' ') ++text;  // the keyboard starts with invisible spaces, for the game's sake
        snprintf(filter, sizeof(filter), "%s", text);
        pick = 0;
        rebuild();
    }
    if (done) {
        typing = false;
        quiet = true;
        linuxSdlInputKeyboardTarget(NULL);
        if (done == 1 && matches == 1) choose();  // enter on the only one left picks it
    }
    unlock();
}
static void startTyping(void) {  // lock held
    filter[0] = 0;
    pick = 0;
    rebuild();
    typing = linuxSdlInputKeyboardAvailable();
    typing_since = 0;
    if (typing) {
        linuxSdlInputKeyboardTarget(typed);
        linuxSdlInputKeyboardRequest(true);
    }
}
static void openForm(View which) {
    form = view = which;
    row = 0;
}
static void pressForm(uint32_t pressed) {
    int rows = formRows();
    row += ((pressed & BTN_DOWN) ? 1 : 0) - ((pressed & BTN_UP) ? 1 : 0);
    row = row < 0 ? 0 : (row > rows - 1 ? rows - 1 : row);
    int step = ((pressed & BTN_RIGHT) ? 1 : 0) - ((pressed & BTN_LEFT) ? 1 : 0);
    if (row == 0 && (pressed & BTN_A)) {
        pick_none = form == VIEW_EDIT;
        view = VIEW_PICK;
        startTyping();
    } else if (row == 1 && step) {  // the nature, with "any" before the first one
        int *nature = formNature();
        *nature += step;
        *nature = *nature < -1 ? (int)BREED_NATURE_COUNT - 1 : (*nature >= (int)BREED_NATURE_COUNT ? -1 : *nature);
    } else if (row >= 2 && row < 8 && (pressed & (BTN_A | BTN_LEFT | BTN_RIGHT))) {
        *formIvs() ^= 1u << (row - 2);
    } else if (form == VIEW_SETUP && row == 8 && (pressed & BTN_A)) {
        stock_row = 0;
        view = VIEW_STOCK;
    } else if (form == VIEW_SETUP && row == 9 && (pressed & BTN_A)) {
        items_back = VIEW_SETUP, item_row = 0;
        view = VIEW_ITEMS;
    } else if (form == VIEW_SETUP && row == 10 && (pressed & BTN_A)) {
        openChains(VIEW_SETUP);
    } else if (form == VIEW_EDIT && row == 8 && (pressed & BTN_A)) {
        ask = ASK_REMOVE, yes = false;
        view = VIEW_CONFIRM;
    }
    if (pressed & BTN_B) {
        if (form == VIEW_SETUP) {
            finish();
        } else {
            row = 0;
            view = VIEW_STOCK;
        }
    }
}
static void pressPick(uint32_t pressed) {  // the list, once the keyboard is gone
    pick += ((pressed & BTN_DOWN) ? 1 : 0) - ((pressed & BTN_UP) ? 1 : 0) + ((pressed & BTN_R) ? 10 : 0) - ((pressed & BTN_L) ? 10 : 0);
    pick = pick > matches - 1 ? matches - 1 : pick;
    pick = pick < 0 ? 0 : pick;
    if (pressed & BTN_Y) startTyping();
    if ((pressed & BTN_A) && matches) choose();
    if (pressed & BTN_B) view = form;
}
static void press(uint32_t pressed) {  // lock held; SDL gamepad buttons
    int steps = (int)plan.count;
    if (view == VIEW_NEXT) {
        if (pressed & BTN_B) atomic_store(&active, false);
        if ((pressed & BTN_A) && !problem && steps) view = VIEW_CONFIRM, ask = ASK_DONE, yes = false;  // the answer starts at No
        if (pressed & BTN_X) view = VIEW_STEPS, scroll = 0;
        if (pressed & BTN_Y) view = VIEW_ITEMS, item_row = 0, items_back = VIEW_NEXT;
        if (pressed & BTN_PLUS) openForm(VIEW_SETUP);
        if (pressed & BTN_MINUS) openChains(VIEW_NEXT);
    } else if (view == VIEW_CONFIRM) {
        if (pressed & BTN_LEFT) yes = false;
        if (pressed & BTN_RIGHT) yes = true;
        if ((pressed & BTN_A) && yes) {
            if (ask == ASK_DONE) {
                breedDone(&state, &plan);
                finish();
            } else if (ask == ASK_REMOVE) {
                memmove(&state.stock[edit], &state.stock[edit + 1], (state.stock_count - (unsigned)edit - 1) * sizeof(BreedStock));
                state.stock_count--;
                stock_row = 0;
                view = VIEW_STOCK;
            } else {
                char path[96];
                snprintf(path, sizeof(path), CHAIN_PATH, chains[chain_row - 1].slot);
                remove(path);
                listChains();
                chain_row = 0;
                view = VIEW_CHAINS;
            }
        } else if (pressed & (BTN_A | BTN_B)) {
            view = ask == ASK_DONE ? VIEW_NEXT : (ask == ASK_CHAIN ? VIEW_CHAINS : VIEW_EDIT);
        }
    } else if (view == VIEW_VISUAL) {
        if (pressed & BTN_B) { // B always closes, never toggles an effect
            atomic_store(&active, false);
            return;
        }
        // Shoulder buttons cycle tabs in either direction; a simultaneous L+R is ignored.
        if (!!(pressed & BTN_L) != !!(pressed & BTN_R)) {
            visual_tab = (visual_tab + ((pressed & BTN_R) ? 1 : LINUX_VISUAL_TABS - 1)) % LINUX_VISUAL_TABS;
            visual_row = 0; // avoid accidentally toggling a different row after switching tabs
            visual_notice = NULL;
            return; // a shoulder press cannot also change a setting in the new tab
        }
        visual_row += !!(pressed & BTN_DOWN) - !!(pressed & BTN_UP);
        if (visual_row < 0) visual_row = 0;
        if ((unsigned)visual_row >= linuxVisualMenuRows((unsigned)visual_tab))
            visual_row = (int)linuxVisualMenuRows((unsigned)visual_tab) - 1;
        if (pressed & BTN_A) {
            unsigned effect = linuxVisualMenuEffect((unsigned)visual_tab, (unsigned)visual_row);
            if (effect < VISUAL_EFFECT_COUNT) {
                if (!linuxVisualEffectToggle(effect))
                    visual_notice = "Setting changed; could not save to the SD card.";
                else
                    visual_notice = NULL;
            }
        }
    } else if (view == VIEW_SHOPPING) {
        if (pressed & BTN_B) atomic_store(&active, false);
    } else if (view == VIEW_STEPS) {
        scroll += ((pressed & BTN_DOWN) ? 1 : 0) - ((pressed & BTN_UP) ? 1 : 0) + ((pressed & BTN_R) ? 10 : 0) - ((pressed & BTN_L) ? 10 : 0);
        scroll = scroll < 0 ? 0 : (scroll > steps - 1 ? (steps > 0 ? steps - 1 : 0) : scroll);
        if (pressed & BTN_B) view = VIEW_NEXT;
    } else if (view == VIEW_ITEMS) {
        item_row += ((pressed & BTN_DOWN) ? 1 : 0) - ((pressed & BTN_UP) ? 1 : 0);
        item_row = item_row < 0 ? 0 : (item_row > (int)BREED_ITEMS - 1 ? (int)BREED_ITEMS - 1 : item_row);
        state.have[item_row] += ((pressed & BTN_RIGHT) ? 1 : 0) - ((pressed & BTN_LEFT) ? 1 : 0);
        state.have[item_row] = state.have[item_row] < 0 ? 0 : (state.have[item_row] > 99 ? 99 : state.have[item_row]);
        if (pressed & BTN_B) {
            if (items_back == VIEW_SETUP)
                view = VIEW_SETUP;
            else
                finish();
        }
    } else if (view == VIEW_STOCK) {
        int count = (int)state.stock_count;
        stock_row += ((pressed & BTN_DOWN) ? 1 : 0) - ((pressed & BTN_UP) ? 1 : 0);
        stock_row = stock_row < 0 ? 0 : (stock_row > count ? count : stock_row);
        if ((pressed & BTN_A) && stock_row == 0 && state.stock_count < BREED_STOCK) {
            edit = count;
            state.stock[count] = (BreedStock){.name = "Pokemon", .nature = -1};
            state.stock_count++;
            openForm(VIEW_EDIT);
        } else if ((pressed & BTN_A) && stock_row > 0) {
            edit = stock_row - 1;
            openForm(VIEW_EDIT);
        }
        if (pressed & BTN_B) openForm(VIEW_SETUP), row = 8;
    } else if (view == VIEW_CHAINS) {
        chain_row += ((pressed & BTN_DOWN) ? 1 : 0) - ((pressed & BTN_UP) ? 1 : 0);
        chain_row = chain_row < 0 ? 0 : (chain_row > chain_count ? chain_count : chain_row);
        if (pressed & (BTN_A | BTN_X | BTN_DOWN | BTN_UP)) notice = NULL;
        if ((pressed & BTN_A) && chain_row == 0)
            newChain();
        else if (pressed & BTN_A)
            switchTo(chain_row - 1);
        if ((pressed & BTN_X) && chain_row > 0) view = VIEW_CONFIRM, ask = ASK_CHAIN, yes = false;
        if (pressed & BTN_B) view = chains_back;
    } else if (view == VIEW_PICK) {
        pressPick(pressed);
    } else {
        pressForm(pressed);
    }
}
void linuxRoadmapFeed(const LinuxInputSnapshot *s) {
    static LinuxInputSnapshot before;
    static bool have_before;
    lock();
    if (!atomic_load(&active)) {
        // Closed: the buttons that closed it must be let go before the game hears of any input again.
        bool calm = !s->fingers && (!s->gamepad || ((s->buttons & 0x7fffu) == 0 && s->axes[0] > -9000 && s->axes[0] < 9000 && s->axes[1] > -9000 && s->axes[1] < 9000));
        if (calm) atomic_store(&hold, false);
        have_before = false;
    } else if (atomic_exchange(&fresh, false) || !have_before) {  // what is held when it opens does not count
        before = *s;
        have_before = true;
    } else {
        if (typing && !typing_since) typing_since = s->timestamp_ns;
        if (typing && s->timestamp_ns - typing_since > 4000000000ull && !linuxSdlInputKeyboardVisible()) {  // the keyboard did not come up, or was closed
            typing = false;
            linuxSdlInputKeyboardTarget(NULL);
            linuxSdlInputKeyboardRequest(false);
        }
        if (quiet) quiet = false, quiet_until = s->timestamp_ns + 600000000ull;
        if (s->gamepad && !typing && s->timestamp_ns >= quiet_until) press(s->buttons & ~before.buttons);
        before = *s;
    }
    unlock();
}

// ---- drawing: the file chooser's rectangles and text, in a 1280x720 view -----------------------------------------------------------
static LinuxRoadmapRect drawRect;
static LinuxRoadmapText drawText;
static LinuxRoadmapClip drawClip;
static void line(float *y, const char *string, float r, float g, float b) {
    // Wrap prose at word boundaries while there is room in the current panel.
    // Rows without remaining vertical space fall back to the clipped, bouncing
    // marquee handled by the shared font renderer, never draw off-window.
    const char *at = string;
    while (*at) {
        size_t length = strlen(at);
        if (length > COLUMNS && *y + 2.0f * ROW <= 683.0f) {
            size_t take = nxTextWrapLine(at, COLUMNS);
            char segment[COLUMNS + 1u];
            memcpy(segment, at, take);
            segment[take] = 0;
            drawText(TEXT_X, *y, segment, COLUMNS, r, g, b);
            *y += ROW;
            at += take;
            while (*at == ' ' || *at == '\t') ++at;
        } else {
            drawText(TEXT_X, *y, at, COLUMNS, r, g, b);
            *y += ROW;
            break;
        }
    }
    if (!*string) *y += ROW;
}
static void panel(float x, float y, float w, float h) {
    if (drawClip) drawClip(x + 10.0f, y + 7.0f, w - 20.0f, h - 14.0f);
    drawRect(x - 3, y - 3, w + 6, h + 6, 0.35f, 0.75f, 0.45f, 1.0f);
    drawRect(x, y, w, h, 0.04f, 0.07f, 0.06f, 0.94f);
}
static void listLine(float *y, bool selected, const char *string) {  // a line of a list, with a bar behind the one that is picked
    if (selected) drawRect(TEXT_X - 10, *y - 3, 1100, 32, 0.2f, 0.4f, 0.3f, 1.0f);
    line(y, string, 1.0f, 1.0f, 1.0f);
}
static void drawNext(void) {
    char text[160], label[120];
    float y = 424;
    panel(30, 410, 1220, 300);
    if (problem || !plan.count) {
        snprintf(text, sizeof(text), "Breeding roadmap - %s", state.species[0] ? state.species : "no target");
        line(&y, text, 1.0f, 0.85f, 0.3f);
        line(&y, problem ? problem : "Target met. Nothing left to breed.", problem ? 1.0f : 0.5f, problem ? 0.45f : 1.0f, problem ? 0.4f : 0.6f);
        y = 664;
        line(&y, "[Y]Items [X]Steps [+]Setup [-]Chains [B]Close", 0.7f, 0.8f, 0.75f);
        return;
    }
    const BreedStep *step = &plan.steps[0];
    snprintf(text, sizeof(text), "Next step - %s   (%u more after this)", state.species, plan.count - 1);
    line(&y, text, 1.0f, 0.85f, 0.3f);
    unsigned owned_parents = 0, owned_items = 0;
    for (unsigned i = 0; i < 2; ++i) {
        breedLabel(&state, &plan, step->parent[i], label, sizeof(label));
        snprintf(text, sizeof(text), "%s  %s", i ? "With  " : "Breed ", label);
        line(&y, text, 1.0f, 1.0f, 1.0f);
        snprintf(text, sizeof(text), "holding %s  [%s]", BREED_ITEM_NAMES[step->item[i]], step->have[i] ? "have" : "need");
        line(&y, text, step->have[i] ? 0.5f : 1.0f, step->have[i] ? 1.0f : 0.45f, step->have[i] ? 0.6f : 0.4f);
        if (step->parent[i].kind == 1) owned_parents++;
        if (step->have[i]) owned_items++;
    }
    snprintf(text, sizeof(text), "Logged stock: %u/2 Pokemon, %u/2 items%s", owned_parents, owned_items,
             owned_parents == 2 && owned_items == 2 ? "  [STOCK READY]" : "");
    line(&y, text, 0.7f, 0.85f, 0.75f);
    breedResult(&state, step, label, sizeof(label));
    snprintf(text, sizeof(text), "Makes  %s", label);
    line(&y, text, 0.6f, 0.85f, 1.0f);
    snprintf(text, sizeof(text), "Egg groups: %s", eggs);
    line(&y, text, 0.7f, 0.8f, 0.75f);
    line(&y, "[A]Done [Y]Items [X]Steps [+]Setup [-]Chains [B]Close", 0.7f, 0.8f, 0.75f);
}
static void drawConfirm(void) {
    panel(240, 230, 800, 190);
    static const char *const TITLE[] = {"Mark this step done?", "Remove this Pokemon?", "Delete this saved chain?"};
    static const char *const TEXT[] = {"The parents you own are used up.", "It is deleted from your stock.", "Its progress is lost for good."};
    drawText(264, 252, TITLE[ask], 35, 1.0f, 0.85f, 0.3f);  // inside the box, which starts at 240
    drawText(264, 292, TEXT[ask], 35, 1.0f, 1.0f, 1.0f);
    if (!yes) drawRect(296, 346, 268, 58, 1, 1, 1, 1);
    if (yes) drawRect(696, 346, 268, 58, 1, 1, 1, 1);
    drawRect(300, 350, 260, 50, 0.25f, 0.32f, 0.36f, 1.0f);
    drawRect(700, 350, 260, 50, 0.8f, 0.1f, 0.1f, 1.0f);  // Yes is the dangerous answer
    drawText(392, 364, "No", 2, 1, 1, 1);
    drawText(780, 364, "Yes", 3, 1, 1, 1);
}
static void drawSteps(void) {
    char text[200], first[40], second[40];
    float y = 34;
    panel(30, 20, 1220, 680);
    snprintf(text, sizeof(text), "All steps (%u)   * = stock ready   [B] back", plan.count);
    line(&y, text, 1.0f, 0.85f, 0.3f);
    for (unsigned i = (unsigned)scroll; i < plan.count && y < 670; ++i) {
        BreedParent parent[2] = {plan.steps[i].parent[0], plan.steps[i].parent[1]};
        char *name[2] = {first, second};
        for (unsigned j = 0; j < 2; ++j) {
            if (parent[j].kind == 2)
                snprintf(name[j], 40, "R%d", parent[j].ref + 1);
            else
                breedLabel(&state, &plan, parent[j], name[j], 40);
        }
        char result[96];
        breedResult(&state, &plan.steps[i], result, sizeof(result));
        bool stock_ready = parent[0].kind == 1 && parent[1].kind == 1 && plan.steps[i].have[0] && plan.steps[i].have[1];
        snprintf(text, sizeof(text), "R%u%s %s: %s + %s", i + 1, stock_ready ? "*" : "", result, first, second);
        line(&y, text, i == 0 ? 0.6f : 1.0f, i == 0 ? 0.85f : 1.0f, 1.0f);
    }
}
// Read-only list for the whole remaining chain, using the exact same green/dark palette as the roadmap.
static void drawShopping(void) {
    char text[160];
    float y = 34;
    int purchases[BREED_ITEMS], total = 0, kinds = 0, owned = 0, required = 0;
    breedShoppingList(&state, &plan, purchases);
    panel(30, 20, 1220, 680);
    snprintf(text, sizeof(text), "Shopping list - %s   [B] close", state.species[0] ? state.species : "no active goal");
    line(&y, text, 1.0f, 0.85f, 0.3f);
    y += ROW;
    if (problem) {
        line(&y, problem, 1.0f, 0.45f, 0.4f);
        line(&y, "Set a goal with R + D-pad right + R3, then [+] Setup.", 0.7f, 0.8f, 0.75f);
        return;
    }
    snprintf(text, sizeof(text), "%u breeding steps remaining  |  Item requirements for the full goal", plan.count);
    line(&y, text, 0.7f, 0.85f, 0.75f);
    y += ROW;
    for (unsigned i = 0; i < BREED_ITEMS; ++i) {
        int have = state.have[i] > 0 ? state.have[i] : 0;
        int used = have < plan.need[i] ? have : plan.need[i];
        required += plan.need[i];
        owned += used;
        total += purchases[i];
        if (!purchases[i]) continue;
        kinds++;
        snprintf(text, sizeof(text), "%-15s BUY %2d     need %2d   have %2d", BREED_ITEM_NAMES[i], purchases[i], plan.need[i], have);
        line(&y, text, 1.0f, 1.0f, 1.0f);
    }
    if (!kinds) line(&y, "All required items are already in your recorded stock.", 0.5f, 1.0f, 0.6f);
    y = 606;
    snprintf(text, sizeof(text), "To buy: %d items   |   In stock: %d/%d   |   %d item types missing", total, owned, required, kinds);
    line(&y, text, total ? 1.0f : 0.5f, total ? 0.85f : 1.0f, 0.3f + (total ? 0.0f : 0.3f));
    line(&y, "Uses manually entered inventory; update it in the main roadmap.", 0.7f, 0.8f, 0.75f);
}
static void drawVisual(void) {
    char label[150];
    const unsigned tab = (unsigned)visual_tab;
    panel(150, 40, 980, 615);
    drawText(180, 64, "NXpanded - Visual Enhancements", 45, 1.0f, 0.85f, 0.3f);
    // Both tabs are always visible and selected with L/R, not touch.
    for (unsigned t = 0; t < LINUX_VISUAL_TABS; ++t) {
        const float x = t == 0 ? 171.0f : 635.0f;
        const bool current = t == tab;
        drawRect(x, 114, 455, 49, current ? 0.19f : 0.08f, current ? 0.39f : 0.15f,
                 current ? 0.28f : 0.12f, 1.0f);
        drawText(x + 14, 126, t == 0 ? "1  Colour & finish" : "2  Soft focus", 22,
                 current ? 1.0f : 0.68f, current ? 0.95f : 0.75f, current ? 0.5f : 0.7f);
    }
    drawText(180, 188, tab == 0 ? "Low-cost everyday effects" : "Higher GPU cost - try one at a time", 49,
             0.7f, 0.85f, 0.75f);
    for (unsigned i = 0; i < linuxVisualMenuRows(tab); ++i) {
        const unsigned effect = linuxVisualMenuEffect(tab, i);
        const bool enabled = linuxVisualEffectEnabled(effect);
        const float y = 247.0f + 69.0f * i;
        if ((int)i == visual_row) drawRect(172, y - 9, 926, 52, 0.20f, 0.40f, 0.30f, 1.0f);
        snprintf(label, sizeof(label), "%s    [ %s ]", linuxVisualEffectName(effect), enabled ? "ON " : "OFF");
        drawText(198, y, label, 43, enabled ? 0.55f : 1.0f,
                 enabled ? 1.0f : 0.85f, enabled ? 0.6f : 0.65f);
    }
    unsigned selected = linuxVisualMenuEffect(tab, (unsigned)visual_row);
    drawText(180, 467, linuxVisualMenuDescription(selected), 50, 0.75f, 0.90f, 0.78f);
    drawText(180, 507, "[L/R] tabs   [Up/Down] select   [A] toggle   [B] close", 52, 1.0f, 1.0f, 1.0f);
    int status = linuxVisualEffectsStatus();
    const char *message = status < 0 ? "Shader unavailable: game image unaffected."
                          : status > 0 ? "Shader ready (effects only when enabled)."
                                       : "Shader loads when an effect is enabled.";
    drawText(180, 553, message, 48, status < 0 ? 1.0f : 0.7f,
             status < 0 ? 0.5f : 0.85f, 0.6f);
    drawText(180, 594, visual_notice ? visual_notice : "Saved: switch/PokeMMO-NXpanded/visual-effects.cfg", 49,
             visual_notice ? 1.0f : 0.7f, visual_notice ? 0.5f : 0.85f, 0.75f);
}
static void drawItems(void) {
    char text[160];
    float y = 34;
    panel(30, 20, 1220, 680);
    line(&y, "Items you own   [Up/Down] pick   [Left/Right] change   [B] back", 1.0f, 0.85f, 0.3f);
    y += ROW;
    for (unsigned i = 0; i < BREED_ITEMS; ++i) {
        snprintf(text, sizeof(text), "%-14s have %2d   need %2d", BREED_ITEM_NAMES[i], state.have[i], plan.need[i]);
        listLine(&y, (int)i == item_row, text);
    }
}
static void drawForm(void) {  // the target (setup) or a Pokemon the player owns (edit)
    char text[160];
    size_t size;
    const char *name = formName(&size);
    float y = 34;
    panel(30, 20, 1220, 680);
    line(&y, form == VIEW_SETUP ? "Setup - the Pokemon you want   [B] back" : "A Pokemon you own   [B] back", 1.0f, 0.85f, 0.3f);
    y += ROW;
    snprintf(text, sizeof(text), "Pokemon   %s", name[0] ? name : "(choose)  [A]");
    listLine(&y, row == 0, text);
    snprintf(text, sizeof(text), "Nature    < %s >", *formNature() >= 0 ? BREED_NATURES[*formNature()] : (form == VIEW_SETUP ? "Any" : "Unknown"));
    listLine(&y, row == 1, text);
    for (unsigned i = 0; i < BREED_STATS; ++i) {
        snprintf(text, sizeof(text), "[%c] 31 IV in %s", (*formIvs() >> i) & 1u ? 'x' : ' ', BREED_STAT_NAMES[i]);
        listLine(&y, row == (int)i + 2, text);
    }
    if (form == VIEW_SETUP) {
        snprintf(text, sizeof(text), "Pokemon you own (%u)  >", state.stock_count);
        listLine(&y, row == 8, text);
        listLine(&y, row == 9, "Items you own  >");
        listLine(&y, row == 10, "Breeding chains  >");
        y = 640;
        snprintf(text, sizeof(text), "Egg groups: %s", eggs);
        line(&y, text, 0.7f, 0.8f, 0.75f);
    } else {
        listLine(&y, row == 8, "Remove this Pokemon");
    }
}
static void drawStock(void) {
    char text[160], stats[48];
    float y = 34;
    int first = stock_row < 8 ? 0 : stock_row - 7;
    panel(30, 20, 1220, 680);
    line(&y, "Pokemon you own   [A] pick   [B] back", 1.0f, 0.85f, 0.3f);
    y += ROW;
    for (int i = first; i <= (int)state.stock_count && i < first + LIST_ROWS + 1; ++i) {
        if (i == 0) {
            listLine(&y, stock_row == 0, "[Add a Pokemon]");
            continue;
        }
        const BreedStock *mon = &state.stock[i - 1];
        stats[0] = 0;
        for (unsigned s = 0, at = 0; s < BREED_STATS; ++s)
            if ((mon->ivs >> s) & 1u) at += (unsigned)snprintf(stats + at, sizeof(stats) - at, "%s%s", at ? "/" : "", BREED_STAT_NAMES[s]);
        snprintf(text, sizeof(text), "%-14.14s 31 %-23.23s %s", mon->name, stats[0] ? stats : "none", mon->nature >= 0 ? BREED_NATURES[mon->nature] : "");
        listLine(&y, stock_row == i, text);
    }
}
static void drawPick(void) {
    char text[160], groups[64] = "";
    int shown = typing ? 7 : LIST_ROWS, first = pick < shown / 2 ? 0 : pick - shown / 2;
    float y = typing ? 24 : 34;
    if (first > matches - shown) first = matches - shown > 0 ? matches - shown : 0;
    if (typing)
        panel(30, 12, 1220, 336);  // the keyboard takes the lower half of the screen
    else
        panel(30, 20, 1220, 680);
    snprintf(text, sizeof(text), typing ? "Type the name   %d match%s   [Enter] picks the last one" : "%d match%s   [Y] type   [A] pick   [B] back", matches, matches == 1 ? "" : "es");
    line(&y, text, 1.0f, 0.85f, 0.3f);
    snprintf(text, sizeof(text), "> %s%s", filter, typing ? "_" : "");
    line(&y, text, 1.0f, 1.0f, 1.0f);
    for (int i = first; i < matches && i < first + shown; ++i) listLine(&y, !typing && i == pick, match[i] < 0 ? "(none)" : breedSpeciesName((unsigned)match[i]));
    if (!matches) line(&y, "No Pokemon starts with that.", 1.0f, 0.45f, 0.4f);
    if (!typing && matches && match[pick] >= 0 && breedSpeciesGroups((unsigned)match[pick], groups, sizeof(groups))) {
        y = 650;
        snprintf(text, sizeof(text), "Egg groups: %s", groups);
        line(&y, text, 0.7f, 0.8f, 0.75f);
    }
}
static void drawChains(void) {
    char text[160], now[100];
    float y = 34;
    int first = chain_row < 7 ? 0 : chain_row - 6;
    panel(30, 20, 1220, 680);
    line(&y, "Breeding chains   [A] switch   [X] delete   [B] back", 1.0f, 0.85f, 0.3f);
    describe(&state, problem ? -1 : (int)plan.count, now, sizeof(now));
    snprintf(text, sizeof(text), "Now: %s", now);
    line(&y, text, 0.6f, 0.85f, 1.0f);
    for (int i = first; i <= chain_count && i < first + LIST_ROWS - 1; ++i) listLine(&y, chain_row == i, i == 0 ? "[Put this one aside and start a new chain]" : chains[i - 1].label);
    if (notice) {
        y = 650;
        line(&y, notice, 1.0f, 0.45f, 0.4f);
    }
}
// Small, translucent HUD: render thread draws it *after* the game and its cursor. No view change or control capture.
// The renderer applies `opacity` to text (the text callback is also used by the full-screen roadmap).
void linuxRoadmapToastDraw(LinuxRoadmapRect rect, LinuxRoadmapText text, LinuxRoadmapClip clip, float opacity) {
    if (opacity <= 0.0f) return;
    lock();
    char buf[160], first[100], second[100];
    const float x = 278.0f, y = 22.0f, w = 974.0f, h = 254.0f, tx = x + 22.0f;
    if (clip) clip(x + 12.0f, y + 6.0f, w - 24.0f, h - 12.0f);
    rect(x - 3, y - 3, w + 6, h + 6, 0.35f, 0.75f, 0.45f, opacity);
    rect(x, y, w, h, 0.04f, 0.07f, 0.06f, 0.94f * opacity);
    text(tx, y + 13, "NXpanded - Quick progress", 43, 1.0f, 0.85f, 0.3f);
    snprintf(buf, sizeof(buf), "Goal: %.23s   |   %u step%s remaining", state.species[0] ? state.species : "(not set)", plan.count, plan.count == 1 ? "" : "s");
    text(tx, y + 45, buf, 43, 0.7f, 0.85f, 0.75f);
    if (problem || !plan.count) {
        text(tx, y + 88, problem ? problem : "Goal complete. No more breeds needed.", 43, 1.0f, 1.0f, 1.0f);
        text(tx, y + 187, "This toast fades away while the game keeps running.", 43, 0.7f, 0.85f, 0.75f);
    } else {
        const BreedStep *step = &plan.steps[0];
        breedLabel(&state, &plan, step->parent[0], first, sizeof(first));
        breedLabel(&state, &plan, step->parent[1], second, sizeof(second));
        snprintf(buf, sizeof(buf), "Breed: %.37s", first);
        text(tx, y + 77, buf, 43, 1.0f, 1.0f, 1.0f);
        snprintf(buf, sizeof(buf), "With:  %.37s", second);
        text(tx, y + 107, buf, 43, 1.0f, 1.0f, 1.0f);
        snprintf(buf, sizeof(buf), "Item 1: %s [%s]", BREED_ITEM_NAMES[step->item[0]], step->have[0] ? "have" : "need");
        text(tx, y + 141, buf, 43, step->have[0] ? 0.55f : 1.0f, step->have[0] ? 1.0f : 0.5f, 0.6f);
        snprintf(buf, sizeof(buf), "Item 2: %s [%s]", BREED_ITEM_NAMES[step->item[1]], step->have[1] ? "have" : "need");
        text(tx, y + 171, buf, 43, step->have[1] ? 0.55f : 1.0f, step->have[1] ? 1.0f : 0.5f, 0.6f);
        snprintf(buf, sizeof(buf), "%u after this breed   |   Your controls remain active", plan.count - 1);
        text(tx, y + 205, buf, 43, 0.7f, 0.85f, 0.75f);
    }
    unlock();
}
void linuxRoadmapDraw(LinuxRoadmapRect rect, LinuxRoadmapText text, LinuxRoadmapClip clip) {
    drawRect = rect;
    drawText = text;
    drawClip = clip;
    lock();
    if (view == VIEW_VISUAL)
        drawVisual();
    else if (view == VIEW_SHOPPING)
        drawShopping();
    else if (view == VIEW_STEPS)
        drawSteps();
    else if (view == VIEW_ITEMS)
        drawItems();
    else if (view == VIEW_SETUP || view == VIEW_EDIT)
        drawForm();
    else if (view == VIEW_STOCK)
        drawStock();
    else if (view == VIEW_PICK)
        drawPick();
    else if (view == VIEW_CHAINS)
        drawChains();
    else if (view == VIEW_CONFIRM) {
        if (ask == ASK_DONE)
            drawNext();
        else if (ask == ASK_CHAIN)
            drawChains();
        else
            drawForm();
        drawConfirm();
    } else
        drawNext();
    unlock();
}
