#include "nxpanded_market.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#define NX_MAX_PRICE 100000000u
#define NX_MARKET_MAX_AGE (7u * 86400u)
static bool parseUnsigned(const char *s,unsigned *out) {
    if(!s||!*s)return false;
    unsigned long long n=0;
    for(;*s;s++) {
        if(!isdigit((unsigned char)*s))return false;
        n=n*10+(unsigned)(*s-'0'); if(n>NX_MAX_PRICE)return false;
    }
    *out=(unsigned)n;return true;
}
static bool parseU64(const char *s,uint64_t *out) {
    if(!s||!*s)return false;
    uint64_t n=0;
    for(;*s;s++) {
        if(!isdigit((unsigned char)*s) || n>(UINT64_MAX-(unsigned)(*s-'0'))/10)return false;
        n=n*10u+(unsigned)(*s-'0');
    }
    *out=n;return true;
}
bool nxMarketLoad(const char *path,NxMarketSnapshot *out) {
    if(!path||!out)return false;
    FILE *f=fopen(path,"rb");if(!f)return false;
    memset(out,0,sizeof(*out));
    char line[256];bool header=false;
    while(fgets(line,sizeof(line),f)) {
        if(!strchr(line,'\n')&&!feof(f)) {
            int ch;while((ch=fgetc(f))!=EOF&&ch!='\n'){};
            out->rejected++;continue;
        }
        line[strcspn(line,"\r\n")]=0;
        if(!*line)continue;
        char *fields[4]={0};fields[0]=line;
        unsigned nf=1;for(char *p=line;*p;p++)if(*p=='\t') {
            *p=0;if(nf<4)fields[nf++]=p+1;else {nf=5;break;}
        }
        if(!header) {
            uint64_t t;
            if(nf!=3||strcmp(fields[0],"#nx-market-v1")||strlen(fields[1])<2||
               strlen(fields[1])>=NX_MARKET_SOURCE_MAX||!parseU64(fields[2],&t)||t<1500000000u)break;
            strcpy(out->source,fields[1]);out->as_of_utc=t;header=true;continue;
        }
        if(line[0]=='#')continue;
        if(out->count>=NX_MARKET_MAX){out->rejected++;continue;}
        NxMarketPrice quote={0};unsigned price=0;
        if(nf==3&&!strcmp(fields[0],"item")) {
            if(!parseUnsigned(fields[1],&quote.item_id)||quote.item_id>=BREED_ITEMS||
               !parseUnsigned(fields[2],&price)||price==0){out->rejected++;continue;}
            quote.species[0]=0;quote.price=price;
        } else if(nf==4&&!strcmp(fields[0],"breeder")) {
            if(!*fields[1]||strlen(fields[1])>=sizeof(quote.species)||
               !parseUnsigned(fields[2],&quote.iv_mask)||quote.iv_mask>=64||
               !parseUnsigned(fields[3],&price)||price==0){out->rejected++;continue;}
            strcpy(quote.species,fields[1]);quote.price=price;
        } else {out->rejected++;continue;}
        out->quotes[out->count++]=quote;
    }
    bool ok=header&&!ferror(f);fclose(f);if(!ok)memset(out,0,sizeof(*out));
    return ok;
}
static uint32_t findItem(const NxMarketSnapshot *market,unsigned item) {
    uint32_t cheapest=UINT32_MAX;
    for(unsigned i=0;i<market->count;i++) {
        const NxMarketPrice *q=&market->quotes[i];
        if(!q->species[0]&&q->item_id==item&&q->price<cheapest)cheapest=q->price;
    }
    return cheapest==UINT32_MAX?0:cheapest;
}
static uint32_t findBreeder(const NxMarketSnapshot *market,unsigned ivs) {
    uint32_t cheapest=UINT32_MAX;
    for(unsigned i=0;i<market->count;i++) {
        const NxMarketPrice *q=&market->quotes[i];
        /* The IV tree records a required IV mask, not which compatible
         * species must be purchased. Never misapply a specific-species
         * GTL quote as the price of an unspecified generic breeder. */
        if(!strcmp(q->species,"*")&&
           (q->iv_mask&ivs)==ivs &&q->price<cheapest)cheapest=q->price;
    }
    return cheapest==UINT32_MAX?0:cheapest;
}
void nxMarketEstimatePlan(const BreedState *state,const BreedPlan *plan,
                          const NxCosts *defaults,const NxMarketSnapshot *market,
                          uint64_t now,NxMarketEstimate *out) {
    memset(out,0,sizeof(*out));
    if(!state||!plan||!defaults)return;
    nxCostEstimate(state,plan,defaults,&out->base);
    /* A snapshot older than seven days is shown but is never silently used as live pricing. */
    out->stale=!market||!market->as_of_utc||!now||now<market->as_of_utc||now-market->as_of_utc>NX_MARKET_MAX_AGE;
    for(unsigned i=0;i<BREED_ITEMS;i++) {
        int count=out->base.outstanding[i];if(count<=0)continue;
        uint32_t quote=(!out->stale&&market)?findItem(market,i):0;
        uint32_t cost=quote?quote:defaults->prices[i];
        if(cost)out->known_total+=(uint64_t)count*cost;else out->unknown_units+=(unsigned)count;
        if(quote)out->matched_units+=(unsigned)count;
    }
    for(unsigned j=0;j<plan->count;j++)for(unsigned k=0;k<2;k++){
        BreedParent parent=plan->steps[j].parent[k];if(parent.kind!=0)continue;
        unsigned mask=parent.ref>=0&&parent.ref<6?1u<<(unsigned)parent.ref:0;
        uint32_t quote=(!out->stale&&market)?findBreeder(market,mask):0;
        uint32_t cost=quote?quote:defaults->prices[BREED_ITEMS];
        if(cost)out->known_total+=cost;else out->unknown_units++;
        if(quote){out->covered_breeders++;out->matched_units++;}
    }
}
