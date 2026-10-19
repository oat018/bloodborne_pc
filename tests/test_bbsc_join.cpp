// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual placement cache, including anonymous game-thread state.
#include "../gpu/bbnet/shadps4/core/bloodborne_re.cpp"
#include <cassert>
#include <cstdio>
#include <sys/mman.h>

extern "C" int runtime_memory_vma_info(uintptr_t, int*, int*, uintptr_t*) { return 0; }
static std::string receipt_query;
int32_t Core::ReadRuntimeSaveReceipt(const char* path, BbNetSaveReceipt* out) {
    receipt_query = path;
    *out = {};
    if (receipt_query != "/savedata0/userdata0001") return ENOENT;
    out->generation = 12;
    std::strcpy(out->path, "/tmp/mounted-character/userdata0001");
    return 0;
}
namespace Common::Log {
bool ShouldLog(Level) { return false; }
void Write(std::string_view, Level, const char*, int, std::string&&) {}
}

static void TestNativeReturn() {
    using namespace Core::Bloodborne;
    // Native selector uses forced map/position for send-home; BBSC join/follow also use it.
    // Capture the live point before joining; native sign/forced fields are overwritten later.
    std::vector<u8> image(0x05600000);
    alignas(16) std::array<u8, 0x1800> global{};
    std::array<u8, 0x130> matching{};
    std::array<u8, 0x90> slots{};
    std::array<u8, 0xB0> player{};
    std::array<u8, 0xA90> root{};
    std::array<u8, 0x30> map_list{}, map_array{};
    std::array<u8, 0xA0> map_entry{};
    std::array<u8, 0x70> local_state{}, local_world{}, model{};
    std::array<u8, 0x10> character{};
    std::array<u8, 0x3C0> character_state{};
    std::array<u8, 0x200> transform{};
    const auto address = [](auto& bytes) { return reinterpret_cast<u64>(bytes.data()); };
    std::array<u64, 2> frame{};
    image_base = reinterpret_cast<u64>(image.data());
    MemoryPatcher::g_eboot_image_size = image.size();
    const auto state = reinterpret_cast<u64>(global.data());
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    WriteValue<u64>(image_base, MatchingStatePointerOffset, reinterpret_cast<u64>(matching.data()));
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, reinterpret_cast<u64>(slots.data()));
    WriteValue<u64>(reinterpret_cast<u64>(slots.data()), 8, reinterpret_cast<u64>(player.data()));
    WriteValue<s32>(address(player), 0xA4, 0);
    WriteValue<u64>(image_base, SummonManagerRootPointerOffset, address(root));
    WriteValue<s32>(address(root), 0xA80, 241010);
    WriteValue<u64>(image_base, CurrentMapListPointerOffset, address(map_list));
    WriteValue<u64>(address(map_list), 0x10, address(map_array));
    WriteValue<s32>(address(map_array), 0x18, 1);
    WriteValue<u64>(address(map_array), 0x20, address(map_entry));
    WriteValue<u32>(address(map_entry), 8, 0x18010000);
    WriteValue<u64>(image_base, CandidateLocalStatePointerOffset, address(local_state));
    WriteValue<u64>(address(local_state), 0x60, address(local_world));
    WriteValue<u64>(address(local_world), 0x58, address(character));
    WriteValue<u64>(address(character), 8, address(character_state));
    WriteValue<u64>(address(character_state), 0x3B0, address(model));
    WriteValue<u64>(address(model), 0x68, address(transform));
    WriteValue<float>(address(transform), 0x1D4, .5f);
    WriteValue<float>(address(transform), 0x1E0, 10);
    WriteValue<float>(address(transform), 0x1E4, 20);
    WriteValue<float>(address(transform), 0x1E8, 30);
    WriteValue<u8>(state, 0x1592, 1);
    CaptureBbscNativeReturnOrigin();
    assert(bbsc_return_origin.has_value());
    assert(bbsc_return_origin->placement.x == 10);
    WriteValue<u8>(state, 0x1592, 0);
    WriteValue<s32>(address(player), 0xA4, 1);
    const std::array<float, 4> home_pos{10, 20, 30, 1};
    const std::array<float, 4> home_rot{0, .5f, 0, 0};
    WriteValue(state, 0x14A0, home_pos);
    WriteValue(state, 0x14B0, home_rot);
    WriteValue<u32>(state, 0x14C0, 0x18010000);
    assert(SetSeamlessHostPlacementHeader("1,18000000,3f800000,40000000,40400000,00000000,240010"));
    BbscWriteConnectFirstPlacement("test join");
    assert(ReadValue<u32>(state, 0x14C0) == 0x18010000);
    assert((ReadValue<std::array<float, 4>>(state, 0x14A0) == home_pos));
    assert((ReadValue<std::array<float, 4>>(state, 0x14B0) == home_rot));
    assert(ReadValue<u32>(state, 0x14C4) == 0x18000000);
    assert(ReadValue<u32>(state, 0x1644) == 0x18000000);

    // Actual p73142 trace: the engine replaces the sign/home block during the join load.
    WriteValue<u32>(state, 0x14C0, 0x18000000);
    WriteValue(state, 0x14A0, std::array<float, 4>{1, 2, 3, 1});
    WriteValue<float>(address(transform), 0x1E0, 999);
    CaptureBbscNativeReturnOrigin(); // kept-party reload/rejoin must not replace the origin
    assert(bbsc_return_origin->placement.x == 10);

    Core::GuestRegisterSnapshot regs{};
    regs.rcx = state;
    regs.rbp = reinterpret_cast<u64>(frame.data());
    WriteValue<u8>(state, 0x1520, 1);
    WriteValue<u32>(state, 0x14F0, 0x18000000);
    // A summon/follow reload must still use the host placement.
    frame[1] = image_base + 0x013C0000;
    GuardBbscStrayForcedWarp(regs);
    assert(ReadValue<u32>(state, 0x14F0) == 0x18000000);
    frame[1] = image_base + 0x0138A51C;
    GuardBbscStrayForcedWarp(regs);
    assert(ReadValue<u32>(state, 0x14F0) == 0x18010000);
    assert((ReadValue<std::array<float, 4>>(state, 0x1500) == home_pos));
    assert((ReadValue<std::array<float, 4>>(state, 0x1510) == home_rot));
    assert(ReadValue<u32>(state, 0x14C0) == 0x18010000);
    assert((ReadValue<std::array<float, 4>>(state, 0x14A0) == home_pos));
    assert(ReadValue<u8>(state, 0x1520) == 1);
    // Invalid stored origin must never become a forced destination.
    bbsc_return_origin->placement.x = std::numeric_limits<float>::quiet_NaN();
    WriteValue<u32>(state, 0x14F0, 0x18000000);
    GuardBbscStrayForcedWarp(regs);
    assert(ReadValue<u32>(state, 0x14F0) == 0x18000000);
    bbsc_return_origin->placement.x = 10;
    // Invaders keep their existing return behavior.
    WriteValue<s32>(reinterpret_cast<u64>(player.data()), 0xA4, 2);
    GuardBbscStrayForcedWarp(regs);
    assert(ReadValue<u32>(state, 0x14F0) == 0x18000000);
    WriteValue<s32>(address(player), 0xA4, 1);
    bbsc_return_origin.reset();
    GuardBbscStrayForcedWarp(regs);
    assert(ReadValue<u32>(state, 0x14F0) == 0x18000000);
    // A new party must capture its latest live point, not reuse the previous one.
    WriteValue<u8>(state, 0x1592, 1);
    WriteValue<s32>(address(player), 0xA4, 0);
    CaptureBbscNativeReturnOrigin();
    assert(bbsc_return_origin->placement.x == 999);
    WriteValue<u32>(address(map_entry), 8, 0x1D000000); // unsupported origin cannot reuse old point
    CaptureBbscNativeReturnOrigin();
    assert(!bbsc_return_origin.has_value());
    bbsc_return_origin.reset();
    // Native 018F9876..018F98B4 clears this discriminator for goods102 and sets it only for111.
    // The cloned11251 must set the same flag instead of inheriting a prior consumable's value.
    std::array<u8, 0x700> item_data{};
    Core::GuestRegisterSnapshot use{};
    use.rbx = address(local_world);
    use.r13 = address(item_data);
    use.r15 = 11251;
    ApplyBbscPartingBlankUseFlag(use);
    assert(item_data[0x680] == 1);
    item_data[0x680] = 0;
    use.r15 = 100;
    ApplyBbscPartingBlankUseFlag(use);
    assert(item_data[0x680] == 0);
    use.r15 = 11251;
    use.rbx = address(player); // remote/non-local character must not be changed
    ApplyBbscPartingBlankUseFlag(use);
    assert(item_data[0x680] == 0);
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    std::puts("PASS: live pre-join origin survives engine overwrite/rejoin; co-op-only return");
    std::puts("PASS: Parting Blank native discriminator, unrelated goods/remote player unchanged");
}

static void TestSelectedSaveSlot() {
    using namespace Core::Bloodborne;
    namespace fs = std::filesystem;
    char temporary[] = "/tmp/bbsc-selected-save-XXXXXX";
    const char* created = mkdtemp(temporary);
    assert(created);
    const fs::path root(created), previous = fs::current_path();
    const char* configured = std::getenv("BBSC_SAVE_DIR");
    const std::optional<std::string> previous_dir = configured
        ? std::optional<std::string>{configured} : std::nullopt;
    fs::current_path(root);
    fs::create_directories(root / "explicit");
    const auto write_slot = [&](const fs::path& dir, int slot, int age) {
        fs::create_directories(dir);
        const auto file = dir / fmt::format("userdata{:04}", slot);
        std::ofstream(file).put('\0');
        fs::last_write_time(file, bbsc_run_started + std::chrono::seconds(age));
    };
    write_slot(root / "explicit", 0, 1);
    write_slot(root / "explicit", 1, 2);
    write_slot(root / "explicit", 10, 3);
    setenv("BBSC_SAVE_DIR", (root / "explicit").c_str(), 1);
    std::vector<u8> image(0x05600000);
    std::array<u8, 0x1800> global{};
    image_base = reinterpret_cast<u64>(image.data());
    MemoryPatcher::g_eboot_image_size = image.size();
    const u64 state = reinterpret_cast<u64>(global.data());
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    BbNetSaveReceipt receipt{};
    WriteValue<s32>(state, 0x1490, 1);
    assert(!ReadBbscSelectedSaveReceipt(receipt) && receipt.generation == 12);
    assert(receipt_query == "/savedata0/userdata0001");
    receipt_query.clear();
    WriteValue<s32>(state, 0x1490, 10);
    assert(ReadBbscSelectedSaveReceipt(receipt) == EINVAL && !receipt.generation);
    assert(receipt_query.empty()); // metadata is never a character save
    WriteValue<s32>(state, 0x1490, 0);
    assert(ReadBbscSelectedSaveReceipt(receipt) == ENOENT && !receipt.generation);
    WriteValue<s32>(state, 0x1490, 0);
    auto selected = BbscActiveSaveSlot();
    assert(selected && selected->slot == 0 && selected->user == "save");
    WriteValue<s32>(state, 0x1490, 1);
    selected = BbscActiveSaveSlot();
    assert(selected && selected->slot == 1);
    for (s32 slot : {-1, 10, 11}) {
        WriteValue<s32>(state, 0x1490, slot);
        assert(!BbscActiveSaveSlot());
    }
    WriteValue<s32>(state, 0x1490, 0);
    fs::remove(root / "explicit/userdata0000");
    assert(!BbscActiveSaveSlot()); // another character's newer save is no substitute
    write_slot(root / "explicit", 0, -1);
    assert(!BbscActiveSaveSlot()); // preserve legacy recovery's current-run write gate
    write_slot(root / "explicit", 0, 1);
    WriteValue<u64>(image_base, GlobalStatePointerOffset, 0);
    assert(!BbscActiveSaveSlot());
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    write_slot(root / "user/savedata/1/CUSA00207/SPRJ0005", 0, 9);
    selected = BbscActiveSaveSlot();
    assert(selected && selected->user == "save"); // explicit profile is authoritative
    setenv("BBSC_SAVE_DIR", (root / "missing").c_str(), 1);
    assert(!BbscActiveSaveSlot()); // never fall through to another profile
    unsetenv("BBSC_SAVE_DIR");
    selected = BbscActiveSaveSlot();
    assert(selected && selected->user == "sd1" && selected->slot == 0);
    write_slot(root / "user/savedata/2/CUSA00207/SPRJ0005", 0, 10);
    assert(!BbscActiveSaveSlot()); // ambiguous owners must not be resolved by timestamp
    fs::remove_all(root / "user/savedata/2");
    write_slot(root / "user/savedata/1004/CUSA00207/SPRJ0005", 0, 11);
    selected = BbscActiveSaveSlot();
    assert(selected && selected->user == "sd1");
    if (previous_dir) {
        setenv("BBSC_SAVE_DIR", previous_dir->c_str(), 1);
    } else {
        unsetenv("BBSC_SAVE_DIR");
    }
    image_base = 0;
    fs::current_path(previous);
    fs::remove_all(root);
    std::puts("PASS: native selected save beats newer files; invalid/foreign/ambiguous paths reject");
}

static void TestNativeSaveEvidence() {
    using namespace Core::Bloodborne;
    std::vector<u8> image(0x05600000), buffer(0x140000);
    std::array<u8, 0x1800> global{};
    std::array<u64, 6> frame{};
    image_base = reinterpret_cast<u64>(image.data());
    MemoryPatcher::g_eboot_image_size = image.size();
    const u64 state = reinterpret_cast<u64>(global.data());
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    WriteValue<s32>(state, 0x1490, 1);
    WriteValue<u8>(state, 0x1592, 1);
    Core::GuestRegisterSnapshot regs{};
    regs.rdi = static_cast<u64>(-1);
    regs.rdx = 1;
    auto evidence = ReadBbscNativeSaveEvidence(NativeSaveRequestOffset, regs);
    assert(evidence && evidence->stage == "request" && evidence->slot == 1 && evidence->mode == 1);
    regs.rdi = 0;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveRequestOffset, regs);
    assert(evidence && evidence->slot == 0); // Explicit deletion is not the active slot.
    std::array<u8, 32> metadata{};
    regs.rcx = reinterpret_cast<u64>(metadata.data());
    regs.rdx = 2;
    evidence = ReadBbscNativeSaveEvidence(0x01686708, regs);
    assert(evidence && evidence->stage == "slot-clear" && evidence->slot == 2);
    assert(metadata[18] == 0); // Observation must not perform the native clear.
    metadata[18] = 1;
    WriteValue<u64>(image_base, GlobalStatePointerOffset, 0);
    evidence = ReadBbscNativeSaveEvidence(NativeSlotClearOffset, regs);
    assert(evidence && evidence->stage == "slot-clear" && evidence->slot == 2);
    assert(metadata[18] == 1); // Character-list deletion does not require a loaded character.
    Core::GuestRegisterSnapshot task_regs{};
    task_regs.rdx = reinterpret_cast<u64>(metadata.data());
    task_regs.rsi = 2;
    for (const u64 offset : {u64{0x01B3F1BE}, u64{0x01B45DFE}}) {
        const auto task_before = task_regs;
        evidence = ReadBbscNativeSaveEvidence(offset, task_regs);
        assert(evidence && evidence->stage == "slot-clear" && evidence->slot == 2);
        assert(metadata[18] == 1 && !std::memcmp(&task_before, &task_regs, sizeof(task_regs)));
        task_regs.rsi = 10;
        assert(!ReadBbscNativeSaveEvidence(offset, task_regs));
        task_regs.rsi = 2;
        task_regs.rdx = 0;
        assert(!ReadBbscNativeSaveEvidence(offset, task_regs));
        task_regs.rdx = reinterpret_cast<u64>(metadata.data());
    }
    assert(!ReadBbscNativeSaveEvidence(NativeSaveRequestOffset, regs));
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    regs.rdx = 10;
    assert(!ReadBbscNativeSaveEvidence(0x01686708, regs));
    regs.rdx = 1;
    regs.rcx = 0;
    assert(!ReadBbscNativeSaveEvidence(0x01686708, regs));
    regs.rdi = state;
    regs.rsi = reinterpret_cast<u64>(buffer.data());
    regs.rdx = buffer.size();
    regs.rcx = 0x123456; // Serializer's fourth argument is an optional size output, not a mode.
    const auto before = regs;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeOffset, regs);
    assert(evidence && evidence->stage == "serialize-begin" && evidence->own_world);
    assert(evidence->buffer == regs.rsi && evidence->bytes == buffer.size());
    assert(evidence->mode == 0);
    assert(!std::memcmp(&before, &regs, sizeof(regs)));
    WriteValue<u8>(state, 0x1592, 0);
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeOffset, regs);
    assert(evidence && !evidence->own_world); // Guest serialization may reuse cached own-world data.
    evidence = ReadBbscNativeSaveEvidence(NativeSaveLoadOffset, regs);
    assert(evidence && evidence->stage == "deserialize" && evidence->slot == 1);
    regs.rdx = 0x40000;
    assert(!ReadBbscNativeSaveEvidence(NativeSaveLoadOffset, regs));
    regs.rdx = buffer.size();
    regs.rsi = 0;
    assert(!ReadBbscNativeSaveEvidence(NativeSaveSerializeOffset, regs));
    regs.rcx = 1;
    regs.r15 = reinterpret_cast<u64>(buffer.data());
    evidence = ReadBbscNativeSaveEvidence(NativeSaveCopyOffset, regs);
    assert(evidence && evidence->stage == "copy-input" && evidence->slot == 1);
    regs.rcx = 10;
    assert(!ReadBbscNativeSaveEvidence(NativeSaveCopyOffset, regs));
    regs.rdi = 2;
    regs.rdx = state;
    WriteValue<u8>(state, 0x1578, 1);
    evidence = ReadBbscNativeSaveEvidence(NativeSlotReserveOffset, regs);
    assert(evidence && evidence->stage == "slot-reserved" && evidence->slot == 2);
    assert(ReadValue<s32>(state, 0x1490) == 1); // Observer must not reserve or overwrite anything.
    WriteValue<u8>(state, 0x1578, 0);
    assert(!ReadBbscNativeSaveEvidence(NativeSlotReserveOffset, regs));
    regs.rbx = 0;
    regs.rsp = 0;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeEndOffset, regs);
    assert(evidence && evidence->success == 0 && evidence->buffer == 0);
    regs.rbx = 1;
    assert(!ReadBbscNativeSaveEvidence(NativeSaveSerializeEndOffset, regs));
    frame[5] = reinterpret_cast<u64>(buffer.data());
    regs.rsp = reinterpret_cast<u64>(frame.data());
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeEndOffset, regs);
    assert(evidence && evidence->success == 1 && evidence->buffer == frame[5]);
    const u64 data = reinterpret_cast<u64>(buffer.data());
    WriteValue<u32>(data, 0, 0x41);
    WriteValue<u32>(data, 0x14, 0x54);
    WriteValue<u32>(data, 0x18, 0x200);
    WriteValue<u64>(data, 0x244, 4127);
    WriteValue<u64>(data, 0x24C, 2723151752040083ULL);
    WriteValue<u32>(data, 0x2C, 0x254);
    WriteValue<u32>(data, 0x30, 8);
    WriteValue<s32>(data, 0x254, 7);
    const auto buffer_before = buffer;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeEndOffset, regs);
    assert(evidence && evidence->snapshot && evidence->snapshot->capped_cycle == 7);
    assert(!evidence->own_world); // Cached guest snapshot is still labelled as visiting.
    const auto serialized_hash = evidence->snapshot->sha256;
    regs.rcx = 2;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveCopyOffset, regs);
    assert(evidence && evidence->slot == 2 && evidence->snapshot->sha256 == serialized_hash);
    buffer.back() = 1;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveCopyOffset, regs);
    assert(evidence->snapshot->sha256 != serialized_hash); // Changed copy input cannot match old output.
    buffer = buffer_before;
    regs.rdi = state;
    regs.rsi = data;
    regs.rdx = buffer.size();
    evidence = ReadBbscNativeSaveEvidence(NativeSaveLoadOffset, regs);
    assert(evidence && evidence->snapshot->sha256 == serialized_hash);
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeOffset, regs);
    assert(evidence && !evidence->snapshot); // Output bytes at begin are not a completed snapshot.
    regs.rbx = 0;
    evidence = ReadBbscNativeSaveEvidence(NativeSaveSerializeEndOffset, regs);
    assert(evidence && !evidence->snapshot); // Failure cannot inherit previous successful output.
    regs.rbx = 1;
    assert(buffer == buffer_before);
    WriteValue<s32>(state, 0x1490, 10);
    assert(!ReadBbscNativeSaveEvidence(NativeSaveSerializeEndOffset, regs));
    image_base = 0;
    std::puts("PASS: read-only native save lifecycle decodes slots/buffers and rejects invalid evidence");
}

static void TestNativeCycleIncrementEvidence() {
    using namespace Core::Bloodborne;
    std::vector<u8> image(0x05600000);
    std::array<u8, 0x1800> global{};
    std::array<u8, 0x90> slots{};
    const u64 state = reinterpret_cast<u64>(global.data());
    const u64 manager = reinterpret_cast<u64>(slots.data());
    image_base = reinterpret_cast<u64>(image.data());
    MemoryPatcher::g_eboot_image_size = image.size();
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, manager);
    WriteValue<s32>(state, 0x1490, 1);
    WriteValue<u8>(state, 0x1592, 1);
    Core::GuestRegisterSnapshot regs{};
    for (const u64 offset : {u64{0x0131D260}, u64{0x01334BB0}, u64{0x017C0FD2}}) {
        for (s32 cycle : {0, 6, 7}) {
            WriteValue<s32>(manager, 0x68, cycle);
            const auto before = slots;
            const auto evidence = ReadBbscNativeSaveEvidence(offset, regs);
            assert(evidence && evidence->stage == "cycle-increment-entry" &&
                   evidence->slot == 1 && evidence->own_world && evidence->live_capped_cycle == cycle);
            assert(evidence->success == -1 && !evidence->snapshot && evidence->buffer == 0);
            assert(slots == before); // The observer never executes or confirms the increment.
        }
        const auto site = std::find_if(Sites.begin(), Sites.end(),
                                      [=](const TraceSite& value) { return value.offset == offset; });
        assert(site != Sites.end() && site->kind == TraceKind::SaveLifecycle);
        for (s32 invalid : {-1, 8}) {
            WriteValue<s32>(manager, 0x68, invalid);
            assert(!ReadBbscNativeSaveEvidence(offset, regs));
        }
        WriteValue<s32>(manager, 0x68, 7);
        WriteValue<u8>(state, 0x1592, 0);
        assert(!ReadBbscNativeSaveEvidence(offset, regs)->own_world);
        WriteValue<u8>(state, 0x1592, 1);
    }
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, 0);
    assert(!ReadBbscNativeSaveEvidence(0x017C0FD2, regs));
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    std::puts("PASS: all three NG increment observers retain pre-increment count without mutations");
}

static void TestSerializedSnapshot() {
    using namespace Core::Bloodborne;
    std::vector<u8> bytes(0x140000);
    bytes.reserve(bytes.size() + 1);
    const u64 data = reinterpret_cast<u64>(bytes.data());
    WriteValue<u32>(data, 0, 0x41);
    WriteValue<u32>(data, 0x14, 0x54);
    WriteValue<u32>(data, 0x18, 0x200);
    WriteValue<u64>(data, 0x244, 4127);
    WriteValue<u64>(data, 0x24C, 2723151752040083ULL);
    WriteValue<u32>(data, 0x2C, 0x254);
    WriteValue<u32>(data, 0x30, 8);
    const auto baseline = bytes;
    const auto snapshot = ReadBbscSerializedSnapshot(bytes);
    assert(snapshot && snapshot->server_user == 4127 &&
           snapshot->server_character == 2723151752040083ULL && snapshot->capped_cycle == 0);
    assert(snapshot->sha256 == "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390");
    bytes.back() = 1; // Same IDs/count, different snapshot: timestamps cannot establish provenance.
    const auto newer = ReadBbscSerializedSnapshot(bytes);
    assert(newer && newer->sha256 != snapshot->sha256);
    bytes = baseline;
    assert(ReadBbscSerializedSnapshot(bytes)->sha256 == snapshot->sha256);
    assert(bytes == baseline); // Reading never stamps the native snapshot.
    for (const size_t size : {size_t{0}, size_t{0x54}, size_t{0x13FFFF}})
        assert(!ReadBbscSerializedSnapshot(std::span<const u8>{bytes.data(), size}));
    bytes.push_back(0);
    assert(!ReadBbscSerializedSnapshot(bytes));
    bytes.pop_back();
    for (const auto [offset, value] : std::array<std::pair<u32, u32>, 11>{
             {{0, 0x42}, {0x14, 0x50}, {0x18, 15}, {0x18, 0xFFFFFFFF},
              {0x2C, 0x24C}, {0x2C, 0xFFFFFFFF}, {0x30, 4}, {0x30, 0xFFFFFFFF},
              {0x254, 0xFFFFFFFF}, {0x254, 8}, {0x244, 0}}}) {
        const auto original = ReadValue<u32>(data, offset);
        WriteValue<u32>(data, offset, value);
        assert(!ReadBbscSerializedSnapshot(bytes));
        WriteValue<u32>(data, offset, original);
    }
    for (const u64 invalid : {0ULL, 0x8000000000000000ULL, 9007199254740992ULL}) {
        WriteValue<u64>(data, 0x24C, invalid);
        assert(!ReadBbscSerializedSnapshot(bytes));
    }
    bytes = baseline;
    WriteValue<u32>(data, 0x254, 7);
    WriteValue<u32>(data, 0x258, 0xDEADBEEF);
    assert(ReadBbscSerializedSnapshot(bytes)->capped_cycle == 7);
    std::puts("PASS: serialized native snapshot validates identity/cycle bounds and hashes all bytes");
}

static void TestNativeCharacterIdentity() {
    using namespace Core::Bloodborne;
    namespace fs = std::filesystem;
    char temporary[] = "/tmp/bbsc-native-identity-XXXXXX";
    assert(mkdtemp(temporary));
    const fs::path folder{temporary}, file = folder / "userdata0001";
    const char* configured = std::getenv("BBSC_SAVE_DIR");
    const std::optional<std::string> previous_dir = configured
        ? std::optional<std::string>{configured} : std::nullopt;
    setenv("BBSC_SAVE_DIR", folder.c_str(), 1);
    std::vector<u8> image(0x05600000), bytes(0x140000);
    std::array<u8, 0x1800> global{};
    std::array<u8, 0x90> slots{};
    std::array<u8, 0x698> local{};
    const auto address = [](auto& data) { return reinterpret_cast<u64>(data.data()); };
    image_base = address(image);
    MemoryPatcher::g_eboot_image_size = image.size();
    WriteValue<u64>(image_base, GlobalStatePointerOffset, address(global));
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, address(slots));
    WriteValue<u64>(address(slots), 8, address(local));
    WriteValue<s32>(address(global), 0x1490, 1);
    WriteValue<u8>(address(global), 0x1592, 1);
    WriteValue<u64>(address(local), 0x688, 4127);
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    WriteValue<u32>(address(bytes), 0, 0x41);
    WriteValue<u32>(address(bytes), 0x14, 0x54);
    WriteValue<u32>(address(bytes), 0x18, 0x200);
    WriteValue<u64>(address(bytes), 0x244, 4127);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040083ULL);
    WriteValue<u32>(address(bytes), 0x2C, 0x254);
    WriteValue<u32>(address(bytes), 0x30, 8);
    const auto save = [&] {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        out.close();
        fs::last_write_time(file, bbsc_run_started - std::chrono::hours(24));
    };
    save();
    const auto baseline = local;
    auto identity = ReadBbscNativeCharacterIdentity();
    assert(identity && identity->slot == 1 && identity->server_user == 4127);
    assert(identity->server_character == 2723151752040083ULL);
    assert(identity->save_file == fs::canonical(file) && identity->capped_cycle == 0);
    assert(identity->persisted_capped_cycle == 0);
    assert(identity->snapshot_sha256 ==
           "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390");
    assert(local == baseline); // Reading identity never repairs or stamps a save.
    WriteValue<s32>(address(slots), 0x68, 7);
    assert(ReadBbscNativeCharacterIdentity()->capped_cycle == 7); // evidence, not an uncapped key
    assert(ReadBbscNativeCharacterIdentity()->persisted_capped_cycle == 0);
    WriteValue<s32>(address(bytes), 0x254, 7);
    WriteValue<u32>(address(bytes), 0x258, 0xDEADBEEF); // adjacent serialized field is not a cycle
    save();
    assert(ReadBbscNativeCharacterIdentity()->persisted_capped_cycle == 7);
    for (s32 persisted_cycle : {-1, 8}) {
        WriteValue<s32>(address(bytes), 0x254, persisted_cycle);
        save();
        assert(!ReadBbscNativeCharacterIdentity());
    }
    WriteValue<s32>(address(bytes), 0x254, 0);
    save();
    for (s32 cycle : {-1, 8}) {
        WriteValue<s32>(address(slots), 0x68, cycle);
        assert(!ReadBbscNativeCharacterIdentity());
    }
    WriteValue<s32>(address(slots), 0x68, 0);
    for (u64 id : {0ULL, 0x8000000000000000ULL, 9007199254740992ULL}) {
        WriteValue<u64>(address(local), 0x690, id);
        assert(!ReadBbscNativeCharacterIdentity());
    }
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    for (u64 user : {0ULL, 0x8000000000000000ULL}) {
        WriteValue<u64>(address(local), 0x688, user);
        assert(!ReadBbscNativeCharacterIdentity());
    }
    WriteValue<u64>(address(local), 0x688, 4127);
    WriteValue<s32>(address(local), 0xA4, 1);
    assert(!ReadBbscNativeCharacterIdentity());
    WriteValue<s32>(address(local), 0xA4, 0);
    for (s32 slot : {-1, 0, 10}) {
        WriteValue<s32>(address(global), 0x1490, slot);
        assert(!ReadBbscNativeCharacterIdentity());
    }
    WriteValue<s32>(address(global), 0x1490, 1);
    WriteValue<u8>(address(global), 8, 1);
    assert(!ReadBbscNativeCharacterIdentity());
    WriteValue<u8>(address(global), 8, 0);
    WriteValue<u8>(address(global), 0x1592, 0);
    assert(!ReadBbscNativeCharacterIdentity());
    WriteValue<u8>(address(global), 0x1592, 1);
    WriteValue<u64>(address(slots), 8, 0);
    assert(!ReadBbscNativeCharacterIdentity());
    WriteValue<u64>(address(slots), 8, address(local));
    for (const auto [offset, value] : std::array<std::pair<u32, u32>, 11>{
             {{0, 0x42}, {0x14, 0x50}, {0x18, 15}, {0x18, 0xFFFFFFFF}, {0x24C, 123},
              {0x2C, 0x50}, {0x2C, 0x24C}, {0x2C, 0x13FFF9}, {0x2C, 0xFFFFFFFF},
              {0x30, 4}, {0x30, 0xFFFFFFFF}}}) {
        const auto original = ReadValue<u32>(address(bytes), offset);
        WriteValue<u32>(address(bytes), offset, value);
        save();
        assert(!ReadBbscNativeCharacterIdentity());
        WriteValue<u32>(address(bytes), offset, original);
    }
    save();
    fs::resize_file(file, 0x100);
    assert(!ReadBbscNativeCharacterIdentity());
    save();
    fs::rename(file, folder / "userdata0000");
    assert(!ReadBbscNativeCharacterIdentity()); // another slot is never a fallback
    fs::create_symlink(folder / "userdata0000", file);
    assert(!ReadBbscNativeCharacterIdentity());
    fs::remove(file);
    fs::rename(folder / "userdata0000", file);
    unsetenv("BBSC_SAVE_DIR");
    assert(!ReadBbscNativeCharacterIdentity());
    if (previous_dir) setenv("BBSC_SAVE_DIR", previous_dir->c_str(), 1);
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    fs::remove_all(folder);
    std::puts("PASS: native identity matches selected persisted slot without mtime; invalid/foreign evidence rejects");
}

static void TestIdentityEvidenceBounds() {
    using namespace Core::Bloodborne;
    std::array<s32, 16> data{};
    data.fill(12345);
    data[4] = 0;
    data[5] = 1;
    data[6] = -1;
    data[7] = 10; // metadata/count candidate, never accepted as a character identity here
    data[8] = 11;
    data[9] = -2;
    const u64 address = reinterpret_cast<u64>(data.data());
    const auto candidate = ReadBbscIdentityCandidates(address, sizeof(data));
    assert(candidate.has_value());
    assert((candidate.value() == std::vector<std::pair<u32, s32>>{{16, 0}, {20, 1}, {24, -1}, {28, 10}}));
    assert(!ReadBbscIdentityCandidates(0, sizeof(data)));
    assert(!ReadBbscIdentityCandidates(address, 0));
    assert(!ReadBbscIdentityCandidates(address, 3));
    assert(!ReadBbscIdentityCandidates(address, 0x104));
    std::puts("PASS: read-only identity evidence bounds and small-integer filtering");
}

static void TestPartingBlankRow() {
    using namespace Core::Bloodborne;
    std::array<u8, 512> table{};
    const auto base = reinterpret_cast<u64>(table.data());
    WriteValue<u16>(base, 10, 2);
    WriteValue<u32>(base, 0x40, 111);
    WriteValue<u64>(base, 0x48, 128);
    WriteValue<u32>(base, 0x58, 11251);
    WriteValue<u64>(base, 0x60, 256);
    WriteValue<s32>(base, 128, 7);
    WriteValue<s32>(base, 128 + 36, 20000432);
    WriteValue<u8>(base, 128 + 62, 15);
    WriteValue<u8>(base, 128 + 63, 4);
    WriteValue<u8>(base, 128 + 68, 0x5F);
    PrepareBbscKitRows(base);
    assert(ReadValue<s32>(base, 256) == 7);
    assert(ReadValue<u8>(base, 256 + 62) == 15);
    assert(ReadValue<u8>(base, 256 + 63) == 4); // native stop-ringing/end-co-op menu
    assert(ReadValue<s32>(base, 256 + 36) == 20000432); // native menu dialog
    assert(ReadValue<u8>(base, 128 + 68) == 0); // original item still retired
    PrepareBbscKitRows(base);
    assert(ReadValue<u8>(base, 256 + 63) == 4);
    std::puts("PASS: Parting Blank retains native stop-ringing/end-co-op menu and effect/animation IDs");
}

static int join_stage_calls = 0;
static int join_warp_calls = 0;
static void PS4_SYSV_ABI TestPlacementSetter(const void*) {}
static void PS4_SYSV_ABI TestWarpFlag() {}
static bool PS4_SYSV_ABI TestPlacementSelector(u64, s32) { return true; }
static u64 PS4_SYSV_ABI TestJoinStage() { ++join_stage_calls; return 1; }
static void PS4_SYSV_ABI TestJoinWarp(u64, float* position, float*, u32) {
    assert(position[0] == 2 && position[1] == 2 && position[2] == 3);
    ++join_warp_calls;
}

static void TestSingleNativeJoinLoad() {
    using namespace Core::Bloodborne;
    constexpr size_t size = 0x05600000;
    void* image = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(image != MAP_FAILED);
    image_base = reinterpret_cast<u64>(image);
    MemoryPatcher::g_eboot_image_size = size;
    const auto stub = [](u64 offset, u64 target) {
        const u64 address = image_base + offset;
        assert(mprotect(reinterpret_cast<void*>(address & ~u64{4095}), 4096,
                        PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
        const std::array<u8, 12> jump{0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
        std::memcpy(reinterpret_cast<void*>(address), jump.data(), jump.size());
        WriteValue<u64>(address, 2, target);
    };
    for (u64 off : {SetForcedSummonPositionOffset, SetForcedSummonOrientationOffset,
                    SetForcedSummonMapOffset}) stub(off, reinterpret_cast<u64>(TestPlacementSetter));
    stub(SetForcedSummonWarpOffset, reinterpret_cast<u64>(TestWarpFlag));
    stub(SelectSummonedPlacementOffset, reinterpret_cast<u64>(TestPlacementSelector));
    stub(StageTransitionOffset, reinterpret_cast<u64>(TestJoinStage));
    stub(0x100000, reinterpret_cast<u64>(TestJoinWarp));
    std::array<std::array<u8, 0x500>, 2> players{};
    std::array<std::array<u8, 0x30>, 2> modules{};
    std::array<std::array<u8, 0x100>, 2> hp{};
    std::array<std::array<u8, 0x10>, 2> entity{};
    std::array<std::array<u8, 0x3C0>, 2> chr_state{};
    std::array<std::array<u8, 0x70>, 2> model{};
    std::array<std::array<u8, 0x200>, 2> transform{};
    std::array<u8, 0x70> world{}, entries{};
    std::array<u8, 0x130> matching{};
    std::array<u8, 0x1800> global{};
    std::array<u8, 0x10> slots{};
    std::array<u8, 0xB0> local_slot{};
    std::array<u8, 0x30> map_list{}, map_array{};
    std::array<u8, 0xA0> map_entry{};
    std::array<u8, 0x610> vtable{};
    const auto addr = [](auto& value) { return reinterpret_cast<u64>(value.data()); };
    WriteValue<u64>(image_base, CandidateLocalStatePointerOffset, addr(world));
    WriteValue<u64>(addr(world), 0x60, addr(players[0]));
    WriteValue<s32>(addr(world), 0x30, 2);
    WriteValue<u64>(addr(world), 0x38, addr(entries));
    WriteValue<u64>(image_base, MatchingStatePointerOffset, addr(matching));
    WriteValue<u64>(image_base, GlobalStatePointerOffset, addr(global));
    WriteValue<u32>(addr(global), 0x0C, 0x18000000);
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, addr(slots));
    WriteValue<u64>(addr(slots), 8, addr(local_slot));
    WriteValue<s32>(addr(local_slot), 0xA4, 1);
    WriteValue<u64>(image_base, CurrentMapListPointerOffset, addr(map_list));
    WriteValue<u64>(addr(map_list), 0x10, addr(map_array));
    WriteValue<s32>(addr(map_array), 0x18, 1);
    WriteValue<u64>(addr(map_array), 0x20, addr(map_entry));
    WriteValue<u32>(addr(map_entry), 8, 0x18000000);
    WriteValue<u64>(addr(vtable), 0x5F8, image_base + 0x100000);
    for (size_t i = 0; i < 2; ++i) {
        WriteValue<u64>(addr(entries), i * 0x38, addr(players[i]));
        WriteValue<u64>(addr(players[i]), 0, addr(vtable));
        WriteValue<s32>(addr(players[i]), 0x78, i == 0 ? 1 : 0);
        WriteValue<u64>(addr(players[i]), 0x3B0, addr(modules[i]));
        WriteValue<u64>(addr(modules[i]), 0x20, addr(hp[i]));
        WriteValue<s32>(addr(hp[i]), 0xF8, 100);
        WriteValue<u64>(addr(players[i]), 0x58, addr(entity[i]));
        WriteValue<u64>(addr(entity[i]), 8, addr(chr_state[i]));
        WriteValue<u64>(addr(chr_state[i]), 0x3B0, addr(model[i]));
        WriteValue<u64>(addr(model[i]), 0x68, addr(transform[i]));
        WriteValue<float>(addr(transform[i]), 0x1E0, 1);
        WriteValue<float>(addr(transform[i]), 0x1E4, 2);
        WriteValue<float>(addr(transform[i]), 0x1E8, 3);
        WriteValue<float>(addr(transform[i]), 0x1EC, 1);
    }
    TickBbscJoinTeleport(10'000); // reset previous-party state
    WriteValue<s32>(addr(matching), 0x124, 6);
    bbsc_tick_map = 0x18010000;
    assert(SetSeamlessHostPlacementHeader("1,18000000,3f800000,40000000,40400000,00000000,240010"));
    assert(BbscConnectFirstHost().has_value());
    assert(GetCurrentPackedMap() == 0x18000000);
    WriteValue<u32>(addr(map_entry), 8, 0x18010000);
    BbscConnectFirstReloadFinished(); // do not consume the target before the host map loads
    assert(BbscConnectFirstHost().has_value());
    WriteValue<u32>(addr(map_entry), 8, 0x18000000);
    BbscConnectFirstReloadFinished(); // first native summon load already reached the host
    assert(!BbscConnectFirstHost().has_value());
    assert(FindRemoteHostCharacter() == addr(players[1]));
    TickBbscJoinTeleport(10'250);
    TickBbscJoinTeleport(12'500);
    assert(join_stage_calls == 0); // no second stage transition after a successful native load
    assert(join_warp_calls == 1 && bbsc_join_teleport_done);
    TickBbscJoinTeleport(15'000);
    assert(join_stage_calls == 0 && join_warp_calls == 1);
    WriteValue<s32>(addr(matching), 0x124, 0);
    TickBbscJoinTeleport(15'250);
    WriteValue<s32>(addr(local_slot), 0xA4, 2); // invaders keep their native spawn
    WriteValue<s32>(addr(matching), 0x124, 6);
    TickBbscJoinTeleport(15'500);
    assert(join_stage_calls == 0 && join_warp_calls == 1);
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    assert(munmap(image, size) == 0);
    std::puts("PASS: cross-map native arrival places guest once without a second reload");
}

int main() {
    using namespace Core::Bloodborne;
    setenv("SHADPS4_BBSC_CONNECT_FIRST", "1", 1);
    setenv("SHADPS4_BBSC_WARP_JOIN", "1", 1); // old launch settings cannot restore the extra load
    setenv("SHADPS4_BBSC_LOAD_COVER", "0", 1);
    setenv("SHADPS4_BBSC_WORLD_SYNC", "0", 1);
    bbsc_tick_map = 0x18010000;
    assert(SetSeamlessHostPlacementHeader("1,18000000,3f800000,40000000,40400000,00000000,240010"));
    assert(bbsc_connect_first_map == 0x18000000);
    // Broker Preparing -> Advertised: accepting the rewritten AreaId returns an empty ACK,
    // without a placement header. The host still needs to search again and claim this sign.
    ClearSeamlessHostPlacementHeader();
    assert(bbsc_connect_first_map == 0x18000000);
    assert(BbscConnectFirstHost().has_value());
    // Empty ACKs must not renew the original deadline or preserve an expired target.
    const auto since = bbsc_connect_first_since_ms.load();
    ClearSeamlessHostPlacementHeader();
    assert(bbsc_connect_first_since_ms == since);
    bbsc_connect_first_since_ms = SteadyNowMs() - BbscConnectFirstTimeoutMs - 1;
    ClearSeamlessHostPlacementHeader();
    assert(bbsc_connect_first_map == 0);
    assert(!BbscConnectFirstHost().has_value());
    assert(!seamless_received_host_placement.has_value());
    // A same-map reply must remove a previous cross-map target immediately.
    assert(SetSeamlessHostPlacementHeader("1,18000000,3f800000,40000000,40400000,00000000,240010"));
    assert(SetSeamlessHostPlacementHeader("1,18010000,3f800000,40000000,40400000,00000000,241010"));
    assert(bbsc_connect_first_map == 0);
    ClearSeamlessHostPlacementHeader();
    assert(!seamless_received_host_placement.has_value());
    std::puts("PASS: cross-map prepare survives empty ACK, expires without refresh, same-map clears");
    TestNativeReturn();
    TestIdentityEvidenceBounds();
    TestSerializedSnapshot();
    TestNativeCharacterIdentity();
    TestSelectedSaveSlot();
    TestNativeSaveEvidence();
    TestNativeCycleIncrementEvidence();
    TestSingleNativeJoinLoad();
    TestPartingBlankRow();
}
