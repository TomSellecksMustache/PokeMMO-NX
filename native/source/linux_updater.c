#include "linux_updater.h"
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#define ROOT "sdmc:/switch/PokeMMO-NXpanded"
#define API_URL "https://api.github.com/repos/TomSellecksMustache/PokeMMO-NXpanded/releases/latest"
#define ASSET_URL "https://github.com/TomSellecksMustache/PokeMMO-NXpanded/releases/download/"
#define MAX_NRO (64u * 1024u * 1024u)
typedef struct { unsigned char *data; size_t used, max; } Memory;
static size_t toMemory(char *ptr, size_t size, size_t count, void *user) {
    Memory *m = user; size_t bytes = size * count;
    if ((size && bytes / size != count) || bytes > m->max - m->used) return 0;
    memcpy(m->data + m->used, ptr, bytes); m->used += bytes;
    return bytes;
}
typedef struct { FILE *file; size_t bytes; bool failed; Sha256Context hash; } Target;
static size_t toFile(char *ptr, size_t size, size_t count, void *user) {
    Target *t=user; size_t n=size*count;
    if ((size && n/size!=count) || n>MAX_NRO-t->bytes || fwrite(ptr,1,n,t->file)!=n) { t->failed=true; return 0; }
    sha256ContextUpdate(&t->hash,ptr,n); t->bytes+=n; return n;
}
static bool fetch(const char *url, curl_write_callback cb, void *dest) {
    CURL *c=curl_easy_init(); if (!c) return false;
    curl_easy_setopt(c,CURLOPT_URL,url);
    curl_easy_setopt(c,CURLOPT_USERAGENT,"NXpanded-update/1.7.0");
    curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);
    curl_easy_setopt(c,CURLOPT_MAXREDIRS,5L);
    curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,8L);
    curl_easy_setopt(c,CURLOPT_TIMEOUT,55L);
    curl_easy_setopt(c,CURLOPT_FAILONERROR,1L);
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(c,CURLOPT_PROTOCOLS_STR,"https");
    curl_easy_setopt(c,CURLOPT_REDIR_PROTOCOLS_STR,"https");
#else
    curl_easy_setopt(c,CURLOPT_PROTOCOLS,(long)CURLPROTO_HTTPS);
    curl_easy_setopt(c,CURLOPT_REDIR_PROTOCOLS,(long)CURLPROTO_HTTPS);
#endif
    curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,cb);
    curl_easy_setopt(c,CURLOPT_WRITEDATA,dest);
    CURLcode error=curl_easy_perform(c);
    long http=0; curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&http);
    curl_easy_cleanup(c);
    return error==CURLE_OK && http==200;
}
static bool parseTag(const char *json, char *tag, size_t cap) {
    const char *p=strstr(json,"\"tag_name\""); if (!p) return false;
    p=strchr(p,':'); if(!p) return false;
    p++; while (*p && isspace((unsigned char)*p)) p++;
    if (*p++!='"') return false;
    const char *end=strchr(p,'"'); size_t n=end?(size_t)(end-p):0;
    if (!n || n>=cap || n>48) return false;
    memcpy(tag,p,n);tag[n]=0;
    if (strncmp(tag,"v1.",3)!=0 || strstr(tag,"-rc.")) return false;
    for(size_t i=0;i<n;i++) if(!isalnum((unsigned char)tag[i]) && tag[i]!='.' && tag[i]!='-') return false;
    return true;
}
static int versionNumbers(const char *tag, int *a,int *b,int *c) {
    return sscanf(tag[0]=='v'?tag+1:tag,"%d.%d.%d",a,b,c)==3;
}
static bool newer(const char *stable) {
    int a=0,b=0,c=0, x=0,y=0,z=0;
    if(!versionNumbers(stable,&a,&b,&c) || !versionNumbers(APP_VERSION,&x,&y,&z))return false;
    if(a!=x)return a>x;
    if(b!=y)return b>y;
    return c>z;
}
static bool readHash(const unsigned char *buf, size_t len, unsigned char hash[32]) {
    if(len<64) return false;
    for(unsigned i=0;i<32;i++) {
        int h=tolower(buf[i*2]),l=tolower(buf[i*2+1]);
        h=(h>='0'&&h<='9')?h-'0':(h>='a'&&h<='f'?h-'a'+10:-1);
        l=(l>='0'&&l<='9')?l-'0':(l>='a'&&l<='f'?l-'a'+10:-1);
        if(h<0||l<0)return false;
        hash[i]=(unsigned char)((h<<4)|l);
    }
    return true;
}
bool linuxUpdaterCheckAndStage(char *message,size_t cap) {
    if(!message||!cap)return false;
    snprintf(message,cap,"Checking GitHub...");
    // Do not socketExit() here: PokeMMO also has a network stack.
    // The launcher has already initialized Switch sockets; do not reset its network state.
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK) { snprintf(message,cap,"HTTPS unavailable.");return false; }
    unsigned char data[24576]={0}; Memory meta={data,0,sizeof(data)-1};
    if(!fetch(API_URL,toMemory,&meta)) { snprintf(message,cap,"Could not check GitHub. Check internet.");return false; }
    data[meta.used]=0;
    char tag[64];
    if(!parseTag((const char*)data,tag,sizeof(tag))) {snprintf(message,cap,"Unexpected GitHub release metadata.");return false;}
    if(!newer(tag)) {snprintf(message,cap,"Latest stable: %s (no newer stable build).",tag);return true;}
    char url[384];
    snprintf(url,sizeof(url),ASSET_URL "%s/PokeMMO.nro.sha256",tag);
    unsigned char sumtext[512]={0}; Memory sum={sumtext,0,sizeof(sumtext)};
    if(!fetch(url,toMemory,&sum)) {snprintf(message,cap,"Release missing checksum; nothing installed.");return false;}
    unsigned char expected[32], actual[32];
    if(!readHash(sumtext,sum.used,expected)) {snprintf(message,cap,"Invalid published checksum.");return false;}
    mkdir(ROOT,0777);mkdir(ROOT "/updates",0777);
    FILE *f=fopen(ROOT "/updates/PokeMMO.nro.tmp","wb");
    if(!f) {snprintf(message,cap,"Cannot write updates folder.");return false;}
    Target t={.file=f};sha256ContextCreate(&t.hash);
    snprintf(url,sizeof(url),ASSET_URL "%s/PokeMMO.nro",tag);
    bool ok=fetch(url,toFile,&t);
    if(fflush(f))ok=false;
    if(fclose(f))ok=false;
    if(t.failed||t.bytes<4096||t.bytes>MAX_NRO)ok=false;
    sha256ContextGetHash(&t.hash,actual);
    if(memcmp(actual,expected,32))ok=false;
    if(ok) {
        FILE *check=fopen(ROOT "/updates/PokeMMO.nro.tmp","rb");
        if(!check)ok=false;
        else {unsigned char header[20]={0};if(fread(header,1,sizeof(header),check)!=sizeof(header)||memcmp(header+16,"NRO0",4))ok=false;fclose(check);}
    }
    if(!ok){remove(ROOT "/updates/PokeMMO.nro.tmp");snprintf(message,cap,"Download/verification failed; original untouched.");return false;}
    // Stage ONLY. Never replace a currently running NRO.
    remove(ROOT "/updates/PokeMMO.nro");
    if(rename(ROOT "/updates/PokeMMO.nro.tmp",ROOT "/updates/PokeMMO.nro")!=0) {
        remove(ROOT "/updates/PokeMMO.nro.tmp");snprintf(message,cap,"Cannot finish staging update.");return false;
    }
    snprintf(message,cap,"%s verified. Exit and install updates/PokeMMO.nro.",tag);
    return true;
}
