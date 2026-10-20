#define _GNU_SOURCE
#include "runtime.h"
#include "gpu/bbgpu.h"
#include "gpu/bbnet/bbnet.h"
#include <assert.h>
#include <dirent.h>
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
    char native_path[1024];
    snprintf(native_path,sizeof(native_path),"%s/userdata0001",root);
    FILE *native_save=fopen(native_path,"wb");
    assert(native_save && fwrite("existing",1,8,native_save)==8 && !fclose(native_save));
    assert(query("/savedata0/userdata0001",&receipt)==ENOENT);
    assert(!strcmp(receipt.path,native_path) && !receipt.generation && !receipt.error &&
        !receipt.bytes && !receipt.sha256[0]); /* Path provenance alone is not a save receipt. */
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

static int check_offline_recovery_loader(void) {
    const char *module_path="out/bbnet-save-host-test.so";
    void *module=dlopen(module_path,RTLD_NOW|RTLD_LOCAL);
    assert(module);
    int (*recovery_init_calls)(void)=(int (*)(void))dlsym(module,"fixture_recovery_init_calls");
    int (*recovery_ready_calls)(void)=(int (*)(void))dlsym(module,"fixture_recovery_ready_calls");
    int (*network_init_calls)(void)=(int (*)(void))dlsym(module,"fixture_network_init_calls");
    assert(recovery_init_calls && recovery_ready_calls && network_init_calls);
    setenv("BB_ONLINE","0",1);
    setenv("BB_NET_MODULE",module_path,1);
    runtime_net_configure("/bloodborne","CUSA03173","Bloodborne","01.09");
    runtime_net_image_ready(0x12340000,0x5600000);
    assert(recovery_init_calls()==1 && recovery_ready_calls()==1);
    assert(network_init_calls()==0);
    assert(!runtime_net_resolve("Nlev7Lg8k3A#E#F")); // offline imports stay in runtime_services
    assert(network_init_calls()==0);
    puts("PASS: offline Bloodborne recovery loads hooks without network module init/imports");
    return 0;
}

static size_t task_count(void) {
    DIR *tasks=opendir("/proc/self/task");
    assert(tasks);
    size_t count=0;
    struct dirent *entry;
    while ((entry=readdir(tasks)))
        if (entry->d_name[0]!='.') ++count;
    closedir(tasks);
    return count;
}

static int check_offline_recovery_module(void) {
    const char *path="out/gpu/libbbnet.so";
    setenv("BB_ONLINE","0",1);
    setenv("BB_NET_MODULE",path,1);
    runtime_net_configure("/bloodborne","CUSA03173","Bloodborne","01.09");
    const size_t before=task_count();
    runtime_net_image_ready(0,0); /* invalid image safely refuses hook installation */
    assert(!runtime_net_resolve("Nlev7Lg8k3A#E#F"));
    assert(task_count()==before); /* recovery-only load must not start NP/auth workers */
    puts("PASS: actual BBSC recovery entry loads offline without network imports or worker threads");
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
    if (argc==2 && !strcmp(argv[1],"--bbsc-recovery-offline")) return check_offline_recovery_loader();
    if (argc==2 && !strcmp(argv[1],"--bbsc-recovery-module")) return check_offline_recovery_module();
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
