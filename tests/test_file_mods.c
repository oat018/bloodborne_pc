/* Exercise actual guest file operations on a linked overlay and writable saves. */
#include "../src/runtime_file.c"
#include <assert.h>
#include <sys/syscall.h>
static int fail_save_flush; /* 1: file, 2: directory */
static int failed_write_fd=-1;
static int fail_save_rename;
static int check_receipt_commit_lock,fail_hash_read,interrupt_hash_read;
ssize_t read(int fd,void *buffer,size_t size) {
    if (interrupt_hash_read) { interrupt_hash_read=0; errno=EINTR; return -1; }
    if (fail_hash_read) { errno=EIO; return -1; }
    return syscall(SYS_read,fd,buffer,size);
}
static int interrupted_fd=-1,interrupted_kind;
int rename(const char *from,const char *to) {
    if (check_receipt_commit_lock && strstr(from,".bbtmp"))
        assert(pthread_mutex_trylock(&receipt_lock)==EBUSY);
    if (fail_save_rename && strstr(from,".bbtmp")) { errno=EACCES; return -1; }
    return syscall(SYS_rename,from,to);
}
ssize_t write(int fd,const void *buffer,size_t size) {
    if (fd==interrupted_fd && interrupted_kind==0) { interrupted_fd=-1; errno=EINTR; return -1; }
    if (fd==failed_write_fd) { errno=ENOSPC; return -1; }
    return syscall(SYS_write,fd,buffer,size);
}
ssize_t pwrite(int fd,const void *buffer,size_t size,off_t offset) {
    if (fd==interrupted_fd && interrupted_kind==1) { interrupted_fd=-1; errno=EINTR; return -1; }
    if (fd==failed_write_fd) { errno=ENOSPC; return -1; }
    return syscall(SYS_pwrite64,fd,buffer,size,offset);
}
int fsync(int fd) {
    if (fd==interrupted_fd && interrupted_kind==2) { interrupted_fd=-1; errno=EINTR; return -1; }
    struct stat info;
    assert(!fstat(fd,&info));
    if ((fail_save_flush==1 && S_ISREG(info.st_mode)) ||
        (fail_save_flush==2 && S_ISDIR(info.st_mode))) { errno=EIO; return -1; }
    return (int)syscall(SYS_fsync,fd);
}
static int32_t guest_errno;
int32_t *runtime_errno(void) { return &guest_errno; }
int32_t runtime_guest_errno(int e) { return e; }
/* Reads into guest memory tell the GPU side (runtime_memory.c); nothing to tell here. */
void runtime_memory_note_write(uintptr_t address, uint64_t size) { (void)address; (void)size; }
uintptr_t runtime_lookup(const RuntimeExport *table,size_t n,const char *name) {
    for (size_t i=0;i<n;++i) if (!strcmp(table[i].name,name)) return (uintptr_t)table[i].function;
    return 0;
}
static void check_atomic_save_failures(const char *root) {
    char folder[1024],path[1100];
    snprintf(folder,sizeof(folder),"%s/saves",root);
    snprintf(path,sizeof(path),"%s/userdata0000",folder);
    assert(!mkdir(folder,0755));
    runtime_file_mount("/savedata0",folder);
    int fd=(int)do_open("/savedata0/userdata0000",0x602,0600); assert(fd>=3);
    BbNetSaveReceipt before={0},after={0};
    assert(runtime_file_save_receipt(path,&before)==ENOENT);
    assert(do_write(fd,"old",3)==3 && !do_close(fd));
    assert(!runtime_file_save_receipt(path,&before) && before.generation>0 && !before.error);
    assert(before.bytes==3 && !strcmp(before.sha256,
        "cba06b5736faf67e54b07b561eae94395e774c517a7d910a54369e1263ccfbd4"));
    assert(!runtime_file_save_receipt("/savedata0/userdata0000",&after));
    assert(after.generation==before.generation && !strcmp(after.path,before.path));
    for (int failure=1;failure<=2;++failure) {
        fd=(int)do_open("/savedata0/userdata0000",0x402,0600); assert(fd>=3);
        assert(do_write(fd,"new",3)==3);
        fail_save_flush=failure;
        int result=(int)do_close(fd);
        fail_save_flush=0;
        assert(result==-EIO); /* a directory flush must not be reported as a committed save */
        assert(!runtime_file_save_receipt(path,&after));
        assert(!after.bytes && !after.sha256[0]);
        assert(after.generation==before.generation && after.error==EIO);
        fd=(int)do_open("/savedata0/userdata0000",0,0); assert(fd>=3);
        char content[4]={0}; assert(do_read(fd,content,3)==3 && !do_close(fd));
        assert(!strcmp(content,failure==1 ? "old" : "new"));
    }
    for (int positional=0;positional<2;++positional) {
        fd=(int)do_open("/savedata0/userdata0000",0x402,0600); assert(fd>=3);
        failed_write_fd=host_fd(fd);
        assert((positional ? do_pwrite(fd,"bad",3,0) : do_write(fd,"bad",3))==-ENOSPC);
        failed_write_fd=-1;
        assert(do_close(fd)==-ENOSPC);
        assert(!runtime_file_save_receipt(path,&after));
        assert(!after.bytes && !after.sha256[0]);
        assert(after.generation==before.generation && after.error==ENOSPC);
        fd=(int)do_open("/savedata0/userdata0000",0,0); assert(fd>=3);
        char content[4]={0}; assert(do_read(fd,content,3)==3 && !do_close(fd));
        assert(!strcmp(content,"new"));
    }
    for (int operation=0;operation<4;++operation) {
        fd=(int)do_open("/savedata0/userdata0000",0x402,0600); assert(fd>=3);
        assert(do_write(fd,"bad",3)==3);
        int expected=operation<2 ? EINVAL : operation==2 ? EIO : EACCES;
        if (operation==0) assert(do_ftruncate(fd,-1)==-EINVAL);
        if (operation==1) assert(do_truncate("/savedata0/userdata0000",-1)==-EINVAL);
        if (operation==2) {
            fail_save_flush=1;
            assert(do_fsync(fd)==-EIO);
            fail_save_flush=0;
        }
        if (operation==3) fail_save_rename=1;
        assert(do_close(fd)==-expected);
        fail_save_rename=0;
        assert(!runtime_file_save_receipt(path,&after));
        assert(!after.bytes && !after.sha256[0]);
        assert(after.generation==before.generation && after.error==expected);
        fd=(int)do_open("/savedata0/userdata0000",0,0); assert(fd>=3);
        char content[4]={0}; assert(do_read(fd,content,3)==3 && !do_close(fd));
        assert(!strcmp(content,"new"));
    }
    runtime_file_unmount("/savedata0");
    assert(!unlink(path) && !rmdir(folder));
    puts("PASS: save close reports file and directory durability failures");
}
static void check_save_receipts(const char *root) {
    char first[1024],second[1024],path[1100],alias[1100];
    snprintf(first,sizeof(first),"%s/receipt-a",root);
    snprintf(second,sizeof(second),"%s/receipt-b",root);
    assert(!mkdir(first,0755) && !mkdir(second,0755));
    assert(!runtime_file_mount("/savedata0",first));
    const char *names[]={"userdata0000","userdata0001","userdata0010"};
    BbNetSaveReceipt a={0},b={0},again={0};
    for (int i=0;i<3;++i) {
        char guest[64]; snprintf(guest,sizeof(guest),"/savedata0/%s",names[i]);
        int fd=(int)do_open(guest,0x602,0600); assert(fd>=3);
        assert(do_write(fd,"one",3)==3 && !do_close(fd));
        snprintf(path,sizeof(path),"%s/%s",first,names[i]);
        if (i==0) assert(!runtime_file_save_receipt(path,&a) && a.generation>0 && !a.error);
        if (i==1) assert(!runtime_file_save_receipt(path,&b) && b.generation>a.generation && !b.error);
        if (i==2) assert(runtime_file_save_receipt(path,&again)==ENOENT && !again.generation);
    }
    snprintf(path,sizeof(path),"%s/userdata0000",first);
    int fd=(int)do_open("/savedata0/userdata0000",2,0600); assert(fd>=3);
    assert(!do_close(fd)); // an unwritten writable copy is not a new saved snapshot
    fd=(int)do_open("/savedata0/userdata0000",2,0600); assert(fd>=3);
    assert(!do_write(fd,"",0) && !do_pwrite(fd,"",0,0) && !do_close(fd));
    fd=(int)do_open("/savedata0/userdata0000",0,0); assert(fd>=3 && !do_close(fd));
    assert(!runtime_file_save_receipt(path,&again) && again.generation==a.generation);
    snprintf(alias,sizeof(alias),"%s/receipt-link",root);
    assert(!symlink(first,alias));
    snprintf(alias,sizeof(alias),"%s/receipt-link/userdata0000",root);
    assert(!runtime_file_save_receipt(alias,&again));
    assert(again.generation==a.generation && !strcmp(again.path,a.path));
    runtime_file_unmount("/savedata0");
    assert(!runtime_file_save_receipt("/savedata0/userdata0000",&again));
    assert(again.generation==a.generation && !strcmp(again.path,a.path));
    assert(do_open("/savedata0/userdata0000",0,0)==-ENOENT);
    assert(!runtime_file_mount("/savedata0",second));
    assert(runtime_file_save_receipt("/savedata0/userdata0000",&again)==ENOENT && !again.generation);
    assert(runtime_file_save_receipt("/savedata0/userdata0001",&again)==ENOENT && !again.generation);
    fd=(int)do_open("/savedata0/userdata0000",0x602,0600); assert(fd>=3);
    assert(do_write(fd,"two",3)==3 && !do_close(fd));
    snprintf(alias,sizeof(alias),"%s/userdata0000",second);
    assert(!runtime_file_save_receipt(alias,&again) && again.generation>b.generation);
    BbNetSaveReceipt mounted={0};
    assert(!runtime_file_save_receipt("/savedata0/userdata0000",&mounted));
    assert(mounted.generation==again.generation && !strcmp(mounted.path,again.path));
    assert(strcmp(again.path,a.path));
    assert(!runtime_file_save_receipt(path,&again) && again.generation==a.generation);
    assert(runtime_file_save_receipt(NULL,&again)==EINVAL && !again.generation);
    assert(runtime_file_save_receipt(path,NULL)==EINVAL);
    runtime_file_unmount("/savedata0");
    assert(!runtime_file_save_receipt("/savedata0/userdata0000",&again));
    assert(again.generation==mounted.generation && !strcmp(again.path,mounted.path));
    assert(do_open("/savedata0/userdata0000",0,0)==-ENOENT);
    char oversized[1100]; memset(oversized,'x',sizeof(oversized));
    memcpy(oversized,"/savedata0/",11); oversized[sizeof(oversized)-1]=0;
    assert(runtime_file_save_receipt(oversized,&again)==ENAMETOOLONG && !again.generation);
    assert(runtime_file_save_receipt("/savedata0/../userdata0000",&again)==EACCES && !again.generation);
    char binding[1100]; snprintf(binding,sizeof(binding),"%s/receipt-link",root);
    assert(!runtime_file_mount("/savedata0",binding));
    runtime_file_unmount("/savedata0");
    assert(!runtime_file_save_receipt("/savedata0/userdata0000",&again));
    assert(again.generation==a.generation && !strcmp(again.path,a.path));
    snprintf(binding,sizeof(binding),"%s/missing-save-root",root);
    assert(!runtime_file_mount("/savedata0",binding));
    assert(runtime_file_save_receipt("/savedata0/userdata0000",&again)==ENOENT && !again.generation);
    runtime_file_unmount("/savedata0");
    assert(runtime_file_save_receipt("/savedata0/userdata0000",&again)==ENOENT && !again.generation);
    assert(!unlink(alias));
    snprintf(alias,sizeof(alias),"%s/receipt-link",root);
    assert(!unlink(alias));
    for (int i=0;i<3;++i) {
        snprintf(path,sizeof(path),"%s/%s",first,names[i]);
        assert(!unlink(path));
    }
    assert(!rmdir(first) && !rmdir(second));
    puts("PASS: save receipts survive unmount, isolate remapped roots/slots/users, exclude non-save closes");
}
static void check_bound_receipt(const char *root) {
    char folder[1024],path[1100];
    snprintf(folder,sizeof(folder),"%s/receipt-bound",root);
    snprintf(path,sizeof(path),"%s/userdata0001",folder);
    assert(!mkdir(folder,0755) && !runtime_file_mount("/savedata0",folder));
    unsigned char payload[100003];
    for (size_t i=0;i<sizeof(payload);++i) payload[i]=(unsigned char)i;
    int fd=(int)do_open("/savedata0/userdata0001",0x602,0600); assert(fd>=3);
    assert(do_write(fd,payload,sizeof(payload))==sizeof(payload));
    interrupt_hash_read=1;
    check_receipt_commit_lock=1;
    assert(!do_close(fd) && !interrupt_hash_read);
    check_receipt_commit_lock=0;
    BbNetSaveReceipt first={0},next={0};
    assert(!runtime_file_save_receipt(path,&first) && !first.error);
    assert(first.bytes==sizeof(payload) && !strcmp(first.sha256,
        "cec3a8fe244db4929c2213d28d360391c86c847e5083efa2000597fb8671dc74"));
    /* A later host edit must not be silently relabelled with the old generation. */
    int host=open(path,O_WRONLY|O_TRUNC); assert(host>=0);
    assert(write(host,"other",5)==5 && !close(host));
    assert(!runtime_file_save_receipt(path,&next));
    assert(next.generation==first.generation && !strcmp(next.sha256,first.sha256));
    fd=(int)do_open("/savedata0/userdata0001",0x602,0600); assert(fd>=3);
    assert(do_write(fd,"new",3)==3);
    fail_hash_read=1;
    assert(!do_close(fd)); // Diagnostic failure cannot prevent the game's save.
    fail_hash_read=0;
    assert(!runtime_file_save_receipt(path,&next));
    assert(next.generation==first.generation && next.error==EIO && !next.bytes && !next.sha256[0]);
    fd=(int)do_open("/savedata0/userdata0001",0,0); assert(fd>=3);
    char content[4]={0}; assert(do_read(fd,content,3)==3 && !do_close(fd));
    assert(!strcmp(content,"new"));
    fd=(int)do_open("/savedata0/userdata0001",0x602,0600); assert(fd>=3);
    assert(do_write(fd,"new",3)==3 && !do_close(fd));
    assert(!runtime_file_save_receipt(path,&next));
    assert(next.generation>first.generation && !next.error && next.bytes==3 &&
        !strcmp(next.sha256,"11507a0e2f5e69d5dfa40a62a1bd7b6ee57e6bcd85c67c9b8431b36fff21c437"));
    runtime_file_unmount("/savedata0");
    assert(!unlink(path) && !rmdir(folder));
    puts("PASS: receipts bind durable generations to exact copy hashes and invalidate failed hash proof");
}
static void check_first_save_failure(const char *root) {
    char folder[1024],path[1100];
    snprintf(folder,sizeof(folder),"%s/receipt-first",root);
    snprintf(path,sizeof(path),"%s/userdata0009",folder);
    assert(!mkdir(folder,0755) && !runtime_file_mount("/savedata0",folder));
    for (int operation=0;operation<3;++operation) {
        int fd=(int)do_open("/savedata0/userdata0009",0x602,0600); assert(fd>=3);
        if (operation==0) failed_write_fd=host_fd(fd);
        assert(do_write(fd,"new",3)==(operation==0 ? -ENOSPC : 3));
        failed_write_fd=-1;
        fail_save_flush=operation==1;
        fail_save_rename=operation==2;
        int expected=operation==0 ? ENOSPC : operation==1 ? EIO : EACCES;
        assert(do_close(fd)==-expected);
        fail_save_flush=fail_save_rename=0;
        BbNetSaveReceipt receipt={0};
        assert(!runtime_file_save_receipt(path,&receipt));
        assert(!receipt.generation && receipt.error==expected);
        assert(access(path,F_OK)==-1 && errno==ENOENT);
    }
    runtime_file_unmount("/savedata0");
    assert(!rmdir(folder));
    puts("PASS: failed first saves keep error receipts without a destination file");
}
static void check_interrupted_save(const char *root) {
    char folder[1024],path[1100];
    snprintf(folder,sizeof(folder),"%s/receipt-eintr",root);
    snprintf(path,sizeof(path),"%s/userdata0000",folder);
    assert(!mkdir(folder,0755) && !runtime_file_mount("/savedata0",folder));
    uint64_t previous=0;
    for (int operation=0;operation<4;++operation) {
        int fd=(int)do_open("/savedata0/userdata0000",0x602,0600); assert(fd>=3);
        interrupted_fd=host_fd(fd); interrupted_kind=operation<3 ? operation : 2;
        assert((operation==1 ? do_pwrite(fd,"new",3,0) : do_write(fd,"new",3))==3);
        if (operation==2) assert(!do_fsync(fd));
        assert(!do_close(fd) && interrupted_fd==-1);
        BbNetSaveReceipt receipt={0};
        assert(!runtime_file_save_receipt(path,&receipt) && !receipt.error);
        assert(receipt.generation>previous);
        previous=receipt.generation;
    }
    runtime_file_unmount("/savedata0");
    assert(!unlink(path) && !rmdir(folder));
    puts("PASS: interrupted write/pwrite/fsync retry and commit successfully");
}
int main(void) {
    char root[]="/tmp/bbport-mod-files-XXXXXX";
    assert(mkdtemp(root));
    char game[512],user[512],source[512],link[512];
    snprintf(game,sizeof(game),"%s/game",root);
    snprintf(user,sizeof(user),"%s/user",root);
    snprintf(source,sizeof(source),"%s/mod.dcx",root);
    snprintf(link,sizeof(link),"%s/game/asset.dcx",root);
    assert(!mkdir(game,0755));
    FILE *f=fopen(source,"w"); assert(f); assert(fputs("modded",f)>=0); assert(!fclose(f));
    assert(!symlink(source,link));
    runtime_file_configure(game,user);
    check_atomic_save_failures(root);
    check_save_receipts(root);
    check_bound_receipt(root);
    check_first_save_failure(root);
    check_interrupted_save(root);
    for (int i=0;i<3;++i) {
        const char *path=i==0 ? "/app0/asset.dcx" : i==1 ? "/hostapp/asset.dcx" : "asset.dcx";
        int fd=(int)do_open(path,0,0); assert(fd>=3);
        char content[8]={0}; assert(do_read(fd,content,6)==6 && !strcmp(content,"modded"));
        GuestStat info; assert(!do_stat(path,&info) && info.size==6);
        assert(!do_close(fd));
        assert(do_open(path,2,0)==-EROFS);
        assert(do_open(path,0x400,0)==-EROFS);
        assert(do_truncate(path,0)==-EROFS);
        assert(path_op(path,2,0)==-EROFS);
        assert(do_rename(path,"/data/moved")==-EROFS);
    }
    int dir=(int)do_open("/app0",0x20000,0); assert(dir>=3);
    char entries[1024]; int64_t count=do_getdents(dir,entries,sizeof(entries),NULL); assert(count>0);
    int found=0;
    for (int64_t p=0;p<count;) {
        uint16_t length; memcpy(&length,entries+p+4,2);
        assert(length);
        if (!strcmp(entries+p+8,"asset.dcx")) { assert(entries[p+6]==8); found=1; }
        p+=length;
    }
    assert(found && !do_close(dir));
    int save=(int)do_open("/data/test-save",0x202,0644); assert(save>=3);
    assert(do_write(save,"save",4)==4 && !do_close(save));
    assert(!path_op("/data/test-save",2,0));
    assert(!unlink(link) && !unlink(source) && !rmdir(game));
    const char *dirs[]={"temp0","download0","data"};
    for (int i=0;i<3;++i) { char p[1024]; snprintf(p,sizeof(p),"%s/%s",user,dirs[i]); assert(!rmdir(p)); }
    assert(!rmdir(user) && !rmdir(root));
    puts("Guest mod files: reads, stat, merged listing, readonly assets and writable saves PASS");
}
