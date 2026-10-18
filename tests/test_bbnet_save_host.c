/* Loaded by the actual runtime loader to exercise the host callback across dlopen. */
#include "gpu/bbnet/bbnet.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static BbNetHost host;
static int ready_calls;

int bbnet_init(const BbNetHost *value) {
    if (!value || value->abi!=BBNET_ABI_VERSION || !value->save_receipt ||
        getenv("BBSC_TEST_REJECT_INIT")) return -1;
    host=*value;
    return 0;
}

void bbnet_image_ready(uint64_t base,uint64_t size) { (void)base; (void)size; ++ready_calls; }
int fixture_ready_calls(void) { return ready_calls; }

int32_t fixture_save_receipt(const char *path,BbNetSaveReceipt *out) {
    return host.save_receipt ? host.save_receipt(path,out) : ENOSYS;
}

static int fixture_net_init(void) { return 0; }
uintptr_t bbnet_resolve(const char *name) {
    return !strcmp(name,"Nlev7Lg8k3A#E#F") ? (uintptr_t)fixture_net_init : 0;
}
