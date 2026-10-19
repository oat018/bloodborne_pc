#define _GNU_SOURCE
#include "runtime.h"
#include "gpu/bbgpu.h"
#include "gpu/bbnet/bbnet.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int check_save_host(int reject) {
    const char *module_path="out/bbnet-save-host-test.so";
    void *module=dlopen(module_path,RTLD_NOW|RTLD_LOCAL);
    assert(module); // This named test must not silently skip a missing fixture.
    typedef int32_t (*Query)(const char*,BbNetSaveReceipt*);
    const Query query=(Query)dlsym(module,"fixture_save_receipt");
    assert(query);
    setenv("BB_ONLINE","1",1);
    setenv("BB_NET_MODULE",module_path,1);
    runtime_start(1);
    if (reject) {
        setenv("BBSC_TEST_REJECT_INIT","1",1);
        assert(!runtime_net_resolve("Nlev7Lg8k3A#E#F"));
        runtime_net_image_ready(0,0);
        int (*ready_calls)(void)=(int (*)(void))dlsym(module,"fixture_ready_calls");
        assert(ready_calls && !ready_calls());
        puts("PASS: a rejected module cannot install image-ready hooks");
        return 0;
    }
    assert(runtime_resolve("Nlev7Lg8k3A#E#F",0)); // Actual loader supplies BbNetHost.
    char root[]="/tmp/bbsc-save-host-XXXXXX";
    assert(mkdtemp(root));
    assert(!runtime_file_mount("/savedata0",root));
    BbNetSaveReceipt receipt={0};
    assert(query("/savedata0/userdata0001",&receipt)==ENOENT);
    int fd=(int)runtime_file_open("/savedata0/userdata0001",0x602,0600);
    assert(fd>=3 && runtime_file_write(fd,"save",4)==4 && !runtime_file_close(fd));
    assert(!query("/savedata0/userdata0001",&receipt));
    assert(receipt.generation>0 && !receipt.error);
    assert(receipt.bytes==4 && !strcmp(receipt.sha256,
        "157dca92e4250458339d4b835250d44c238f3355e1b7986195188ee434e9baff"));
    char path[1024]; snprintf(path,sizeof(path),"%s/userdata0001",root);
    assert(!strcmp(receipt.path,path));
    const uint64_t generation=receipt.generation;
    assert(query("/savedata0/userdata0000",&receipt)==ENOENT && !receipt.generation);
    assert(!query(path,&receipt) && receipt.generation==generation);
    runtime_file_unmount("/savedata0");
    assert(!query("/savedata0/userdata0001",&receipt) && receipt.generation==generation);
    assert(runtime_file_open("/savedata0/userdata0001",0,0)==-ENOENT);
    assert(!unlink(path) && !rmdir(root));
    puts("PASS: ABI-3 loader supplies actual mounted save receipts to the module");
    return 0;
}

/* The settings menu of the GPU library restarts through probe.c, which tests do not link. */
void runtime_restart(void) { abort(); }

/* Online play (BB_ONLINE=1): the game's network and PSN imports resolve to the online module
 * (out/gpu/libbbnet.so); trophies stay with the runtime. Without the module (a build without its
 * libraries) there is nothing to check. */
int main(int argc,char **argv) {
    if (argc==2 && !strcmp(argv[1],"--bbsc-save-host")) return check_save_host(0);
    if (argc==2 && !strcmp(argv[1],"--bbsc-reject-host")) return check_save_host(1);
    static const char *const online[]={
        "Nlev7Lg8k3A#E#F",  /* sceNetInit */
        "A9cVMUtEp4Y#K#L",  /* sceHttpInit */
        "hdpVEUDFW3s#L#M",  /* sceSslInit */
        "2rsFmlGWleQ#R#S",  /* sceNpCheckNpAvailability */
        "10t3e5+JPnU#P#Q",  /* sceNpMatching2Initialize */
        "3KOuC4RmZZU#T#U",  /* sceNpSignalingInitialize */
    };
    static const char *const local="XbkjbobZlCY#l#m"; /* sceNpTrophyCreateContext */
    const char *path="out/gpu/libbbnet.so";
    void *module=dlopen(path,RTLD_NOW|RTLD_LOCAL); /* the same handle the runtime gets */
    if (!module) {
        printf("SKIP: no online module (%s)\n",dlerror());
        return 0;
    }
    const BbNetResolveFn resolve=(BbNetResolveFn)dlsym(module,"bbnet_resolve");
    assert(resolve);
    const BbNetInitFn init=(BbNetInitFn)dlsym(module,"bbnet_init");
    assert(init);
    const BbNetHost old_host={.abi=1};
    assert(init(&old_host)!=0); // Rejected before reading fields beyond an ABI-1 caller.
    const BbNetHost abi2_host={.abi=2};
    assert(init(&abi2_host)!=0); // Receipt size changed; never call an ABI-2 callback.
    BbNetHost incomplete={.abi=BBNET_ABI_VERSION,.thread_spawn=runtime_thread_spawn,
        .thread_join=runtime_thread_join_spawned};
    assert(init(&incomplete)!=0);
    setenv("BB_ONLINE","1",1);
    setenv("BB_NET_MODULE",path,1);
    runtime_start(1);
    bbgpu_register_kernel();
    for (size_t i=0;i<sizeof(online)/sizeof(*online);++i) {
        uintptr_t address=runtime_resolve(online[i],0);
        if (!address || address!=resolve(online[i])) {
            printf("FAIL: %s (%s) is not the online version\n",online[i],runtime_import_name(online[i]));
            return 1;
        }
    }
    uintptr_t trophy=runtime_resolve(local,0);
    assert(trophy && trophy!=resolve(local));
    printf("PASS: online imports go to the online module, trophies stay local\n");
    return 0;
}
