#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "policy.h"
typedef struct {
    const Policy *policies;
    DWORD values[16][2];
    int writes[16][2];
    int missing, fail_write, mismatch, reads;
} Fake;
static DWORD read_value(void *ctx, const GUID *scheme, const Policy *p, int ac, DWORD *value) {
    Fake *f=ctx;
    int i=(int)(p-f->policies);
    (void)scheme;
    ++f->reads;
    if(i==f->missing) return ERROR_FILE_NOT_FOUND;
    *value=f->values[i][ac];
    if(i==f->mismatch && f->writes[i][ac]) ++*value;
    return 0;
}
static DWORD write_value(void *ctx, const GUID *scheme, const Policy *p, int ac, DWORD value) {
    Fake *f=ctx;
    int i=(int)(p-f->policies);
    (void)scheme;
    if(i==f->fail_write) return ERROR_ACCESS_DENIED;
    ++f->writes[i][ac]; f->values[i][ac]=value; return 0;
}
static PolicyBackend setup(Fake *f) {
    size_t count;
    memset(f,0,sizeof(*f)); f->missing=f->fail_write=f->mismatch=-1;
    f->policies=policy_list(&count); assert(count<=16);
    for(size_t i=0;i<count;++i) f->values[i][0]=f->values[i][1]=100;
    return (PolicyBackend){f,read_value,write_value,0};
}
int main(void) {
    Fake f;
    GUID scheme={0};
    PolicyBackend b=setup(&f);
    size_t count;
    const Policy *p=policy_list(&count);
    assert(count==11);
    assert(!policy_configure(&scheme,&b));
    assert(f.values[0][0]==5 && f.values[1][0]==60 && f.values[2][0]==0);
    assert(f.values[3][0]==100); /* Keep an already stronger efficiency preference. */
    for(size_t i=0;i<count;++i) {
        assert(f.writes[i][0]==1);
        assert(f.writes[i][1]==!p[i].dc_only);
    }
    b=setup(&f); f.values[0][0]=0; f.values[1][0]=40; f.values[3][0]=20;
    f.values[4][0]=20; f.values[5][0]=10; f.values[8][0]=60;
    assert(!policy_configure(&scheme,&b));
    assert(f.values[0][0]==0 && f.values[1][0]==40 && f.values[3][0]==80);
    assert(f.values[4][0]==20 && f.values[5][0]==10 && f.values[8][0]==60);
    b=setup(&f); f.missing=0;
    assert(policy_configure(&scheme,&b)==ERROR_FILE_NOT_FOUND && !f.writes[0][0]);
    b=setup(&f); f.missing=3;
    assert(!policy_configure(&scheme,&b) && !f.writes[3][0] && f.writes[4][0]);
    b=setup(&f); f.fail_write=3;
    assert(policy_configure(&scheme,&b)==ERROR_ACCESS_DENIED && !f.writes[4][0]);
    b=setup(&f); f.mismatch=1;
    assert(policy_configure(&scheme,&b)==ERROR_WRITE_FAULT && !f.writes[2][0]);
    puts("PASS: 11 policies, stronger existing savings preserved, DC-only writes, optional/required support, denied writes and readback mismatch.");
    return 0;
}
