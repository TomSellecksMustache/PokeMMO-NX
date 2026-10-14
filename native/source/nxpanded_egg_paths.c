#include "nxpanded_egg_paths.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static bool valid(const char *str, size_t n) {
    if (!str || !*str || strlen(str) >= n) return false;
    for (; *str; str++) if ((unsigned char)*str < 32 || (unsigned char)*str == 127) return false;
    return true;
}
static bool groupMatch(const char *a, const char *b) {
    char ga[100], gb[100];
    if (!breedEggGroups(a,ga,sizeof(ga)) || !breedEggGroups(b,gb,sizeof(gb))) return false;
    if (!strcasecmp(a,"Ditto") || !strcasecmp(b,"Ditto") ||
        strstr(ga,"Undiscovered") || strstr(gb,"Undiscovered")) return false;
    /* Compare complete group tokens, not partial substrings. */
    for (char *pa=ga; *pa;) {
        while (*pa==' ' || *pa=='/') pa++;
        char *end=pa; while (*end && *end!='/') end++;
        char *trim=end; while (trim>pa && trim[-1]==' ') trim--;
        size_t len=(size_t)(trim-pa);
        if (len>0 && len<40) {
            for (char *pb=gb; *pb;) {
                while (*pb==' ' || *pb=='/') pb++;
                char *stop=pb; while (*stop && *stop!='/') stop++;
                char *e=stop; while (e>pb && e[-1]==' ') e--;
                if ((size_t)(e-pb)==len && !strncasecmp(pa,pb,len)) return true;
                pb=stop;
            }
        }
        pa=end;
    }
    return false;
}
bool nxEggRoutesLoad(const char *filename, NxEggRoutes *routes) {
    if (!filename || !routes) return false;
    FILE *file=fopen(filename,"rb"); if (!file) return false;
    memset(routes,0,sizeof(*routes));
    char row[360];
    while (fgets(row,sizeof(row),file)) {
        if (*row=='#' || *row=='\n') continue;
        if (!strchr(row,'\n') && !feof(file)) {
            int c; while ((c=fgetc(file))!='\n' && c!=EOF) {};
            routes->rejected++; continue;
        }
        char *fields[5]={0}, *cursor=row;
        unsigned pieces=0; fields[pieces++]=cursor;
        for (; *cursor; cursor++) if (*cursor=='\t') {
            *cursor=0;
            if (pieces<5) fields[pieces++]=cursor+1;
            else {pieces=6; break;}
        }
        if (pieces!=5) {routes->rejected++;continue;}
        fields[4][strcspn(fields[4],"\r\n")]=0;
        if (!valid(fields[0],40) || !valid(fields[1],NX_EGG_MOVE_LEN) ||
            !valid(fields[2],40) || strcasecmp(fields[0],fields[2])==0 ||
            (strcmp(fields[3],"known") && strcmp(fields[3],"inherited")) ||
            strcmp(fields[4],"verified") || !groupMatch(fields[0],fields[2]) ||
            routes->count>=NX_EGG_ROUTES_MAX) {routes->rejected++;continue;}
        NxEggEdge *e=&routes->edge[routes->count++];
        snprintf(e->receiver,sizeof(e->receiver),"%s",fields[0]);
        snprintf(e->move,sizeof(e->move),"%s",fields[1]);
        snprintf(e->donor,sizeof(e->donor),"%s",fields[2]);
        e->donor_knows_move=!strcmp(fields[3],"known");
    }
    bool good=!ferror(file); fclose(file);
    return good;
}
static bool owns(const BreedState *stock, const char *species) {
    if (!stock) return false;
    for (unsigned i=0;i<stock->stock_count && i<BREED_STOCK;i++)
        if (!strcasecmp(stock->stock[i].name,species)) return true;
    return false;
}
static void walk(const NxEggRoutes *routes,const BreedState *stock,const char *species,const char *move,
                 NxEggPath *trial,NxEggPath *best) {
    if (trial->hops>=NX_EGG_ROUTE_HOPS) return;
    for (unsigned i=0;i<routes->count;i++) {
        const NxEggEdge *edge=&routes->edge[i];
        if (strcasecmp(edge->receiver,species) || strcasecmp(edge->move,move)) continue;
        bool cycle=false;
        for(unsigned j=0;j<=trial->hops;j++)
            if(!strcasecmp(trial->species[j],edge->donor)) cycle=true;
        if(cycle)continue;
        NxEggPath next=*trial;
        snprintf(next.species[next.hops+1],40,"%s",edge->donor);
        next.hops++;
        if(edge->donor_knows_move) {
            next.owned_source=owns(stock,edge->donor);
            if(!best->hops || next.hops<best->hops ||
               (next.hops==best->hops && next.owned_source && !best->owned_source)) *best=next;
        } else walk(routes,stock,edge->donor,move,&next,best);
    }
}
bool nxEggFindPath(const NxEggRoutes *routes,const BreedState *stock,const char *target,
                   const char *move,NxEggPath *path) {
    if(!routes||!target||!move||!path||!valid(target,40)||!valid(move,NX_EGG_MOVE_LEN))return false;
    memset(path,0,sizeof(*path));
    NxEggPath trial={0};snprintf(trial.species[0],40,"%s",target);
    walk(routes,stock,target,move,&trial,path);
    return path->hops>0;
}
