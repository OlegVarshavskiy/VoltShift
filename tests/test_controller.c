#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "controller.h"

typedef struct {
    ControlState saved;
    GUID current;
    int has_state, clone, configured;
    int saves, duplicates, activates, deletes, clears;
    int fail_save, fail_configure, fail_activate, fail_delete, fail_clear;
    int corrupt, incomplete_activation, fail_duplicate, fail_active;
    int captures, effects, effect_active, fail_capture, fail_effects, fail_restore_effects;
} Fake;
static GUID guid(unsigned value) { GUID g = {0}; g.Data1 = value; return g; }
static DWORD load(void *ctx, ControlState *s) {
    Fake *f=ctx;
    if (f->corrupt) return ERROR_INVALID_DATA;
    if (!f->has_state) return ERROR_FILE_NOT_FOUND;
    *s=f->saved; return 0;
}
static DWORD save(void *ctx, const ControlState *s) {
    Fake *f=ctx;
    ++f->saves;
    if (f->fail_save == f->saves) return ERROR_DISK_FULL;
    f->saved=*s; f->has_state=1; return 0;
}
static DWORD clear(void *ctx) {
    Fake *f=ctx;
    ++f->clears;
    if (f->fail_clear) return ERROR_ACCESS_DENIED;
    f->has_state=0; return 0;
}
static DWORD active(void *ctx, GUID *g) {
    Fake *f=ctx;
    if(f->fail_active) return ERROR_ACCESS_DENIED;
    *g=f->current; return 0;
}
static DWORD new_guid(void *ctx, GUID *g) { (void)ctx; *g=guid(2); return 0; }
static DWORD duplicate(void *ctx, const GUID *src, const GUID *dst) {
    Fake *f=ctx;
    ++f->duplicates;
    assert(f->has_state && !f->saved.ready); /* durable backup precedes any write */
    assert(src->Data1==1 && dst->Data1==2);
    if(f->fail_duplicate) return ERROR_ALREADY_EXISTS;
    f->clone=1; return 0;
}
static DWORD configure(void *ctx, const GUID *g) {
    Fake *f=ctx;
    assert(f->clone && g->Data1==2 && f->current.Data1==1);
    if(f->fail_configure) return ERROR_ACCESS_DENIED;
    f->configured=1; return 0;
}
static DWORD activate(void *ctx, const GUID *g) {
    Fake *f=ctx;
    ++f->activates;
    if(g->Data1==2) assert(f->has_state && f->saved.ready && f->configured);
    if (f->fail_activate) return ERROR_ACCESS_DENIED;
    if(!f->incomplete_activation) f->current=*g;
    return 0;
}
static DWORD delete_scheme(void *ctx, const GUID *g) {
    Fake *f=ctx;
    ++f->deletes;
    assert(g->Data1==2 && f->current.Data1!=2);
    if(f->fail_delete) return ERROR_ACCESS_DENIED;
    if(!f->clone) return ERROR_FILE_NOT_FOUND;
    f->clone=0; return 0;
}
static DWORD capture(void *ctx, ControlState *s) {
    Fake *f=ctx;
    ++f->captures;
    assert(!f->has_state && !s->ready && s->revision==5);
    return f->fail_capture ? ERROR_GEN_FAILURE : 0;
}
static DWORD effects(void *ctx, const ControlState *s, int battery) {
    Fake *f=ctx;
    (void)s;
    ++f->effects;
    if (battery && f->fail_effects) return ERROR_GEN_FAILURE;
    if (!battery && f->fail_restore_effects) return ERROR_ACCESS_DENIED;
    f->effect_active=battery;
    return 0;
}
static ControlBackend setup(Fake *f) {
    memset(f,0,sizeof(*f)); f->current=guid(1);
    return (ControlBackend){f,load,save,clear,active,new_guid,duplicate,configure,activate,delete_scheme,capture,effects};
}
int main(void) {
    Fake f;
    ControlBackend b=setup(&f);
    assert(!control_apply(&b,0) && !f.has_state && !f.duplicates);
    assert(!control_apply(&b,1) && f.has_state && f.saved.ready && f.current.Data1==2);
    assert(!control_apply(&b,1) && f.duplicates==1 && f.activates==1);
    assert(!control_apply(&b,0) && f.current.Data1==1 && f.has_state);
    assert(!control_apply(&b,1) && f.current.Data1==2 && f.duplicates==1);
    assert(!control_restore(&b) && f.current.Data1==1 && !f.has_state && !f.clone);
    assert(!control_restore(&b));

    b=setup(&f); f.fail_capture=1;
    assert(control_apply(&b,1)==ERROR_GEN_FAILURE && !f.has_state && !f.duplicates);
    b=setup(&f); f.fail_effects=1;
    assert(control_apply(&b,1)==ERROR_GEN_FAILURE && f.current.Data1==1 && !f.has_state && !f.clone);
    b=setup(&f); assert(!control_apply(&b,1)); f.fail_restore_effects=1;
    assert(control_restore(&b)==ERROR_ACCESS_DENIED && f.current.Data1==1 && f.has_state && f.clone);
    f.fail_restore_effects=0; assert(!control_restore(&b) && !f.effect_active);
    b=setup(&f); f.has_state=1; f.saved.original=guid(1); f.saved.battery=guid(2);
    f.saved.ready=1; f.saved.revision=4; f.clone=1; f.configured=1; f.current=guid(2);
    assert(!control_apply(&b,1) && f.saved.revision==5 && f.current.Data1==2 && f.captures==1);
    assert(!control_restore(&b));

    b=setup(&f); f.fail_save=1;
    assert(control_apply(&b,1)==ERROR_DISK_FULL && !f.duplicates && !f.activates);
    b=setup(&f); f.fail_save=2;
    assert(control_apply(&b,1)==ERROR_DISK_FULL && !f.has_state && !f.clone && f.current.Data1==1);
    b=setup(&f); f.fail_configure=1;
    assert(control_apply(&b,1)==ERROR_ACCESS_DENIED && !f.has_state && !f.clone && f.current.Data1==1);
    b=setup(&f); f.fail_duplicate=1;
    assert(control_apply(&b,1)==ERROR_ALREADY_EXISTS && !f.deletes && !f.has_state);
    b=setup(&f); f.fail_activate=1;
    assert(control_apply(&b,1)==ERROR_ACCESS_DENIED && !f.has_state && !f.clone);
    b=setup(&f); f.incomplete_activation=1;
    assert(control_apply(&b,1)==ERROR_WRITE_FAULT && !f.has_state && !f.clone);

    b=setup(&f); assert(!control_apply(&b,1)); f.fail_activate=1;
    assert(control_restore(&b)==ERROR_ACCESS_DENIED && f.has_state && f.clone);
    f.fail_activate=0; assert(!control_restore(&b));
    b=setup(&f); assert(!control_apply(&b,1)); f.fail_delete=1;
    assert(control_restore(&b)==ERROR_ACCESS_DENIED && f.current.Data1==1 && f.has_state);
    f.fail_delete=0; assert(!control_restore(&b));
    b=setup(&f); assert(!control_apply(&b,1)); f.fail_clear=1;
    assert(control_restore(&b)==ERROR_ACCESS_DENIED && f.current.Data1==1 && f.has_state && !f.clone);
    f.fail_clear=0; assert(!control_restore(&b));

    b=setup(&f); f.corrupt=1;
    assert(control_apply(&b,1)==ERROR_INVALID_DATA && !f.duplicates && !f.saves);
    assert(control_restore(&b)==ERROR_INVALID_DATA && !f.activates && !f.deletes);
    b=setup(&f); f.fail_active=1;
    assert(control_apply(&b,1)==ERROR_ACCESS_DENIED && !f.has_state);
    b=setup(&f); f.has_state=1; f.saved.original=guid(1); f.saved.battery=guid(2);
    assert(control_apply(&b,1)==ERROR_RECOVERY_FAILURE && !f.activates);
    assert(!control_restore(&b) && !f.has_state); /* crash before duplicate */
    b=setup(&f); f.has_state=1; f.saved.original=guid(1); f.saved.battery=guid(2); f.clone=1;
    assert(!control_restore(&b) && !f.has_state && !f.clone); /* crash after duplicate */
    assert(control_auto_target(0)==0 && control_auto_target(1)==1);
    assert(control_auto_target(255)==-1 && control_auto_target(-1)==-1 && control_auto_target(2)==-1);
    {
        int previous = -1;
        b=setup(&f);
        assert(!control_auto_step(&b,255,&previous) && previous==-1 && !f.has_state);
        assert(!control_auto_step(&b,1,&previous) && previous==1 && !f.has_state);
        assert(!control_auto_step(&b,0,&previous) && previous==0 && f.current.Data1==2);
        assert(!control_auto_step(&b,0,&previous) && f.activates==1);
        assert(!control_auto_step(&b,255,&previous) && previous==0 && f.current.Data1==2);
        assert(!control_auto_step(&b,1,&previous) && previous==1 && f.current.Data1==1);
        f.fail_activate=1;
        assert(control_auto_step(&b,0,&previous)==ERROR_ACCESS_DENIED && previous==1);
        assert(!f.has_state && f.current.Data1==1);
    }
    puts("PASS: profile transactions, journaling order, repeated apply, rollback, failed restore/retry, crash recovery, unknown power source.");
    return 0;
}
