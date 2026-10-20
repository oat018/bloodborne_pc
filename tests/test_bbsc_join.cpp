// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual placement cache, including anonymous game-thread state.
#include "../gpu/bbnet/shadps4/core/bloodborne_re.cpp"
#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>

namespace Libraries::Np::NpSignaling::Helpers {
static std::vector<Core::Bbsc::Party::Peer> bbsc_test_established_peers;
static std::vector<std::pair<Core::Bbsc::Party::Peer, std::vector<u8>>> bbsc_test_sent_packets;
std::optional<Core::Bbsc::Party::Peer> FindBbscPartyPeer(std::string_view online_id) {
    std::optional<Core::Bbsc::Party::Peer> found;
    for (const auto& peer : bbsc_test_established_peers) {
        if (peer.online_id != online_id) continue;
        if (found) return std::nullopt;
        found = peer;
    }
    return found;
}
u32 SendBbscPartyToPeer(const Core::Bbsc::Party::Peer& peer, const void* packet, u32 size) {
    if (!packet) return 0;
    const auto* bytes = static_cast<const u8*>(packet);
    bbsc_test_sent_packets.emplace_back(peer, std::vector<u8>(bytes, bytes + size));
    return 1;
}
} // namespace Libraries::Np::NpSignaling::Helpers

namespace {
std::vector<Libraries::Np::NpMatching2::BbscAdmissionTicket> bbsc_test_admission_tickets;
std::vector<std::pair<Libraries::Np::NpMatching2::BbscAdmissionTicket, bool>>
    bbsc_test_admission_resolutions;
Libraries::Http::BbscSummonCapabilityProofSnapshot bbsc_test_capability_proofs;
bool bbsc_test_admission_resolution_success = true;
}

namespace Libraries::Np::NpMatching2 {
std::vector<BbscAdmissionTicket> GetPendingBbscAdmissionTickets() {
    return bbsc_test_admission_tickets;
}
bool ResolveBbscAdmission(const BbscAdmissionTicket& ticket, bool admitted) {
    bbsc_test_admission_resolutions.emplace_back(ticket, admitted);
    std::erase_if(bbsc_test_admission_tickets, [&](const auto& pending) {
        return pending.context == ticket.context && pending.incarnation == ticket.incarnation &&
            pending.request == ticket.request && pending.room == ticket.room &&
            pending.nonce == ticket.nonce;
    });
    return bbsc_test_admission_resolution_success;
}
} // namespace Libraries::Np::NpMatching2

namespace Libraries::Http {
BbscSummonObservationSnapshot GetBbscSummonObservationSnapshot() {
    return bbsc_test_capability_proofs.observation;
}
std::optional<BbscSummonCapabilityProofSnapshot> GetBbscSummonCapabilityProofs(
    const std::array<u8, 16>&) {
    return bbsc_test_capability_proofs;
}
} // namespace Libraries::Http

extern "C" int runtime_memory_vma_info(uintptr_t, int*, int*, uintptr_t*) { return 0; }
static std::string receipt_query;
static u64 mock_receipt_generation = 12;
static std::string mock_receipt_sha = "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390";
static std::string mock_receipt_override_path;
static bool mock_receipt_path_only;
static int recovery_selector_calls;
static int recovery_stage_calls;
static bool recovery_selector_succeeds{true};
static bool PS4_SYSV_ABI TestRecoveryPlacementSelector(u64 context, s32 warp_info) {
    assert(context == 0 && warp_info == -1);
    ++recovery_selector_calls;
    return recovery_selector_succeeds;
}
static s32 PS4_SYSV_ABI TestRecoveryStageTransition() {
    ++recovery_stage_calls;
    return 1;
}
int32_t Core::ReadRuntimeSaveReceipt(const char* path, BbNetSaveReceipt* out) {
    receipt_query = path;
    *out = {};
    if (receipt_query != "/savedata0/userdata0001") return ENOENT;
    out->generation = mock_receipt_generation;
    out->bytes = 0x140000;
    std::strncpy(out->sha256, mock_receipt_sha.c_str(), sizeof(out->sha256) - 1);
    if (!mock_receipt_override_path.empty()) {
        std::strncpy(out->path, mock_receipt_override_path.c_str(), sizeof(out->path) - 1);
        if (mock_receipt_path_only) {
            out->generation = out->bytes = 0;
            out->sha256[0] = '\0';
            return ENOENT;
        }
        return 0;
    }
    const char* save_dir = std::getenv("BBSC_SAVE_DIR");
    if (!save_dir) return ENOENT;
    const auto save_path = (std::filesystem::path{save_dir} / "userdata0001").string();
    std::strncpy(out->path, save_path.c_str(), sizeof(out->path) - 1);
    return 0;
}

static void PS4_SYSV_ABI IgnoreGuestCodeHook(u64, const Core::GuestRegisterSnapshot*) {}
namespace Common::Log {
bool ShouldLog(Level) { return false; }
void Write(std::string_view, Level, const char*, int, std::string&&) {}
}
namespace Common::FS {
const std::filesystem::path& GetUserPath(PathType type) {
    (void)type;
    static std::filesystem::path path;
    const char* configured = std::getenv("BBSC_SAVE_DIR");
    path = configured ? std::filesystem::path{configured} / "captures" : std::filesystem::path{"/tmp"};
    return path;
}
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
    std::array<u8, 0x70> local_state{}, model{};
    std::array<u8, 0x3C0> local_world{};
    std::array<u8, 0x10> character{};
    std::array<u8, 0x3C0> character_state{};
    std::array<u8, 0x30> player_modules{};
    std::array<u8, 0x100> player_data{};
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
    WriteValue<u64>(image_base, 0x0553B148, address(map_list));
    WriteValue<u64>(address(map_list), 0x10, address(map_array));
    WriteValue<s32>(address(map_array), 0x18, 1);
    WriteValue<u64>(address(map_array), 0x20, address(map_entry));
    WriteValue<u32>(address(map_entry), 8, 0x18010000);
    WriteValue<u64>(image_base, CandidateLocalStatePointerOffset, address(local_state));
    WriteValue<u64>(address(local_state), 0x60, address(local_world));
    WriteValue<u64>(address(local_world), 0x58, address(character));
    WriteValue<u64>(address(local_world), 0x3B0, address(player_modules));
    WriteValue<u64>(address(player_modules), 0x20, address(player_data));
    WriteValue<s32>(address(player_data), 0xF8, 1000);
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
    assert(receipt.bytes == 0x140000 && std::string_view(receipt.sha256) ==
           "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390");
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

static void TestNativeEventFlagProof() {
    using namespace Core::Bloodborne;
    constexpr size_t image_size = 0x05600000;
    auto* image = static_cast<u8*>(mmap(nullptr, image_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(image != MAP_FAILED);
    std::memset(image, 0, image_size);
    std::vector<u8> save(0x140000), storage(17 * 125);
    std::array<u8, 0x1800> global{};
    std::array<u8, 0x90> player_slots{};
    std::array<u8, 0x700> player{};
    std::array<u8, 0x40> manager{}, head{};
    std::array<std::array<u8, 0x40>, 18> nodes{};
    std::array<u8, 0x200> callback_stack{}, serialize_stack{};
    image_base = reinterpret_cast<u64>(image);
    MemoryPatcher::g_eboot_image_size = image_size;
    constexpr std::array<u8, 7> begin_signature{
        0x49, 0x8B, 0x7D, 0x00, 0x48, 0x85, 0xFF};
    constexpr std::array<u8, 5> callback_call{0xE8, 0x4F, 0xF1, 0xE2, 0xFF};
    constexpr std::array<u8, 6> callback_entry{0x55, 0x48, 0x89, 0xE5, 0x41, 0x57};
    constexpr std::array<u8, 7> end_signature{0x8B, 0x84, 0x24, 0x30, 0x01, 0x00, 0x00};
    std::memcpy(image + NativeEventFlagsSerializeBeginOffset, begin_signature.data(), begin_signature.size());
    std::memcpy(image + NativeEventFlagsSerializerCallOffset, callback_call.data(), callback_call.size());
    std::memcpy(image + NativeEventFlagsSerializerOffset, callback_entry.data(), callback_entry.size());
    std::memcpy(image + NativeEventFlagsSerializeEndOffset, end_signature.data(), end_signature.size());
    assert(BbscNativeFlagProofCallbackSignatureValid());
    constexpr std::array<u8, 7> hook_begin_signature{
        0x49, 0x8B, 0x7D, 0x00, 0x48, 0x85, 0xFF};
    constexpr std::array<u8, 7> hook_end_signature{
        0x8B, 0x84, 0x24, 0x30, 0x01, 0x00, 0x00};
    assert(InstallGuestCodeHook(image + NativeEventFlagsSerializeBeginOffset,
                                hook_begin_signature, NativeEventFlagsSerializeBeginOffset,
                                IgnoreGuestCodeHook));
    assert(InstallGuestCodeHook(image + NativeEventFlagsSerializeEndOffset,
                                hook_end_signature, NativeEventFlagsSerializeEndOffset,
                                IgnoreGuestCodeHook));
    assert(BbscNativeFlagProofCallbackSignatureValid()); // Both installed E9 hooks leave attribution intact.
    const long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);
    auto* call_page = reinterpret_cast<void*>(
        (reinterpret_cast<std::uintptr_t>(image + NativeEventFlagsSerializerCallOffset) /
         static_cast<std::uintptr_t>(page_size)) * static_cast<std::uintptr_t>(page_size));
    assert(mprotect(call_page, static_cast<size_t>(page_size),
                    PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
    image[NativeEventFlagsSerializerCallOffset + 1] ^= 1;
    assert(!BbscNativeFlagProofCallbackSignatureValid()); // Patched call target cannot activate proof.
    bbsc_native_flag_proof = BbscNativeFlagProof{};
    ObserveBbscNativeFlagProof(NativeSaveSerializeOffset, {});
    assert(!bbsc_native_flag_proof); // Observer drops proof state when callback attribution is invalid.
    image[NativeEventFlagsSerializerCallOffset + 1] ^= 1;
    assert(mprotect(call_page, static_cast<size_t>(page_size), Core::GuestCodeProtection()) == 0);
    assert(BbscNativeFlagProofCallbackSignatureValid());
    const u64 full_image_size = MemoryPatcher::g_eboot_image_size;
    for (const u64 truncated_size : {
             NativeEventFlagsSerializerCallOffset + callback_call.size() - 1,
             NativeEventFlagsSerializerOffset + callback_entry.size() - 1}) {
        MemoryPatcher::g_eboot_image_size = truncated_size;
        assert(!BbscNativeFlagProofCallbackSignatureValid()); // Both attribution signatures are bounded.
    }
    MemoryPatcher::g_eboot_image_size = full_image_size;
    const u64 state = reinterpret_cast<u64>(global.data());
    const u64 slots = reinterpret_cast<u64>(player_slots.data());
    const u64 man = reinterpret_cast<u64>(manager.data());
    const u64 head_ptr = reinterpret_cast<u64>(head.data());
    const u64 save_ptr = reinterpret_cast<u64>(save.data());
    static_assert(BbscNativeProofGroupKeyCount == 17);
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    WriteValue<u64>(image_base, WorldFlagManPointerOffset, man);
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, slots);
    WriteValue<u64>(slots, 0x08, reinterpret_cast<u64>(player.data()));
    WriteValue<s32>(slots, 0x68, 7);
    WriteValue<s32>(reinterpret_cast<u64>(player.data()), 0xA4, 0);
    WriteValue<u64>(reinterpret_cast<u64>(player.data()), 0x688, 4127);
    WriteValue<u64>(reinterpret_cast<u64>(player.data()), 0x690, 2723151752040083ULL);
    WriteValue<s32>(state, 0x1490, 1);
    WriteValue<u8>(state, 0x1592, 1);
    WriteValue<u32>(man, 0x1C, 1000);
    WriteValue<u32>(man, 0x20, 125);
    WriteValue<u32>(man, 0x24, 17);
    WriteValue<u64>(man, 0x28, reinterpret_cast<u64>(storage.data()));
    WriteValue<u64>(man, 0x38, head_ptr);
    WriteValue<u64>(head_ptr, 0x08, reinterpret_cast<u64>(nodes[0].data()));
    WriteValue<u8>(head_ptr, 0x19, 1);
    for (size_t i = 0; i < nodes.size(); ++i) {
        const u64 node = reinterpret_cast<u64>(nodes[i].data());
        WriteValue<u64>(node, 0x00, head_ptr);
        WriteValue<u64>(node, 0x08, i == 0 ? head_ptr : reinterpret_cast<u64>(nodes[i - 1].data()));
        WriteValue<u64>(node, 0x10, i + 1 == nodes.size() ? head_ptr
                                                         : reinterpret_cast<u64>(nodes[i + 1].data()));
        WriteValue<u8>(node, 0x19, 0);
        WriteValue<u32>(node, 0x20, i == 0 ? 10 : BbscNativeProofGroupKeys[i - 1]);
        WriteValue<s32>(node, 0x28, i == 0 ? 2 : 1); // Unrelated kind-2 entry must be ignored.
        WriteValue<u32>(node, 0x30, static_cast<u32>(i - 1));
    }
    WriteValue<u64>(head_ptr, 0x08, reinterpret_cast<u64>(nodes[0].data()));

    const std::string previous_path = mock_receipt_override_path;
    const bool previous_path_only = mock_receipt_path_only;
    const u64 previous_generation = mock_receipt_generation;
    const std::string previous_sha = mock_receipt_sha;
    const char* previous_save_dir_env = std::getenv("BBSC_SAVE_DIR");
    const std::optional<std::string> previous_save_dir = previous_save_dir_env
        ? std::optional<std::string>{previous_save_dir_env} : std::nullopt;
    const std::filesystem::path proof_dir = "/tmp/bbsc-native-event-flag-proof-test";
    std::error_code proof_ec;
    std::filesystem::remove_all(proof_dir, proof_ec);
    proof_ec.clear();
    std::filesystem::create_directories(proof_dir, proof_ec);
    assert(!proof_ec);
    const std::filesystem::path proof_save_path = proof_dir / "userdata0001";
    mock_receipt_override_path = proof_save_path.string();
    assert(setenv("BBSC_SAVE_DIR", proof_dir.c_str(), 1) == 0);
    mock_receipt_path_only = true;
    mock_receipt_generation = 0;
    bbsc_native_flag_proof.reset();
    Core::GuestRegisterSnapshot regs{};
    regs.rdi = state;
    regs.rsi = save_ptr;
    regs.rdx = save.size();
    ObserveBbscNativeFlagProof(NativeSaveSerializeOffset, regs);
    assert(bbsc_native_flag_proof && bbsc_native_flag_proof->buffer == save_ptr &&
           bbsc_native_flag_proof->slot == 1);

    constexpr u32 start = 0x300;
    constexpr u32 span_bytes = 17 * 125 + 1;
    const u64 stack_ptr = reinterpret_cast<u64>(callback_stack.data());
    WriteValue<u64>(stack_ptr, 0x110, save_ptr);
    WriteValue<u32>(stack_ptr, 0x118, 0x140000);
    WriteValue<u32>(stack_ptr, 0x130, start);
    WriteValue<u32>(stack_ptr, 0xD4, start);
    regs = {};
    regs.rsp = stack_ptr;
    ObserveBbscNativeFlagProof(NativeEventFlagsSerializeBeginOffset, regs);
    assert(bbsc_native_flag_proof && bbsc_native_flag_proof->callback_started &&
           bbsc_native_flag_proof->groups.size() == 17 &&
           bbsc_native_flag_proof->groups.front().key == 12101 &&
           bbsc_native_flag_proof->groups.front().slot == 0 &&
           bbsc_native_flag_proof->groups.back().key == 53501 &&
           bbsc_native_flag_proof->groups.back().slot == 16);
    for (u32 i = 0; i < span_bytes; ++i) save[start + i] = static_cast<u8>(i);
    save[start + 100] &= static_cast<u8>(~0x80); // Candidate 12101800 is false in slot 0.
    save[start + 125 + 100] |= 0x80; // Candidate 12201800 is true in slot 1.
    constexpr u32 final_candidate_group_slot = 15; // Sorted selected key 50000 maps to slot 15.
    constexpr u32 final_candidate_byte = (50000305 % 1000) / 8;
    constexpr u32 final_candidate_bit = 7 - (50000305 % 8);
    save[start + final_candidate_group_slot * 125 + final_candidate_byte] |=
        static_cast<u8>(1u << final_candidate_bit); // 50000305: remainder 305, MSB-first bit 6.
    WriteValue<u32>(stack_ptr, 0x130, start + span_bytes);
    regs.r13 = 1;
    ObserveBbscNativeFlagProof(NativeEventFlagsSerializeEndOffset, regs);
    assert(bbsc_native_flag_proof && bbsc_native_flag_proof->span_offset == start &&
           bbsc_native_flag_proof->span_bytes == span_bytes &&
           bbsc_native_flag_proof->span_sha256.size() == 64);

    WriteValue<u32>(save_ptr, 0, 0x41);
    WriteValue<u32>(save_ptr, 0x14, 0x54);
    WriteValue<u32>(save_ptr, 0x18, 0x200);
    WriteValue<u64>(save_ptr, 0x244, 4127);
    WriteValue<u64>(save_ptr, 0x24C, 2723151752040083ULL);
    WriteValue<u32>(save_ptr, 0x2C, 0x254);
    WriteValue<u32>(save_ptr, 0x30, 8);
    WriteValue<s32>(save_ptr, 0x254, 7);
    const auto snapshot = ReadBbscSerializedSnapshot(save);
    assert(snapshot);
    WriteValue<u64>(reinterpret_cast<u64>(serialize_stack.data()), 0x28, save_ptr);
    regs = {};
    regs.rsp = reinterpret_cast<u64>(serialize_stack.data());
    regs.rbx = 1;
    ObserveBbscNativeFlagProof(NativeSaveSerializeEndOffset, regs);
    assert(bbsc_native_flag_proof && bbsc_native_flag_proof->awaiting_receipt &&
           bbsc_native_flag_proof->server_user == 4127 &&
           bbsc_native_flag_proof->server_character == 2723151752040083ULL &&
           bbsc_native_flag_proof->save_sha256 == snapshot->sha256);
    mock_receipt_path_only = false;
    mock_receipt_generation = 1;
    mock_receipt_sha = std::string(64, '0');
    TickBbscNativeFlagProofReceipt();
    assert(!bbsc_native_flag_proof->receipt_confirmed); // A different complete-save hash is not proof.
    assert(!ReadBbscProvenNativeEventFlag(12201800)); // Unconfirmed spans never produce false or true.
    {
        std::ofstream output(proof_save_path, std::ios::binary | std::ios::trunc);
        assert(output && output.write(reinterpret_cast<const char*>(save.data()), save.size()));
    }
    mock_receipt_generation = 2;
    mock_receipt_sha = snapshot->sha256;
    TickBbscNativeFlagProofReceipt();
    assert(bbsc_native_flag_proof->receipt_confirmed &&
           bbsc_native_flag_proof->captured_span.size() == span_bytes);
    assert(ReadBbscProvenNativeEventFlag(12101800) == std::optional<bool>{false});
    assert(ReadBbscProvenNativeEventFlag(12201800) == std::optional<bool>{true});
    assert(ReadBbscProvenNativeEventFlag(50000305) == std::optional<bool>{true});
    assert(!ReadBbscProvenNativeEventFlag(12101999)); // Outside the approved catalog.
    save[start + 100] |= 0x80;
    save[start + 125 + 100] &= static_cast<u8>(~0x80);
    assert(ReadBbscProvenNativeEventFlag(12101800) == std::optional<bool>{false} &&
           ReadBbscProvenNativeEventFlag(12201800) == std::optional<bool>{true}); // Buffer reuse cannot rewrite evidence.

    mock_receipt_override_path = (proof_dir / "userdata0002").string();
    assert(!ReadBbscProvenNativeEventFlag(12201800)); // A changed current receipt path is unknown.
    mock_receipt_override_path = proof_save_path.string();
    WriteValue<s32>(state, 0x1490, 2);
    assert(!ReadBbscProvenNativeEventFlag(12201800)); // A changed selected slot is unknown.
    WriteValue<s32>(state, 0x1490, 1);
    WriteValue<u64>(reinterpret_cast<u64>(player.data()), 0x690, 2723151752040084ULL);
    assert(!ReadBbscProvenNativeEventFlag(12201800)); // A changed live character is unknown.
    WriteValue<u64>(reinterpret_cast<u64>(player.data()), 0x690, 2723151752040083ULL);
    assert(ReadBbscProvenNativeEventFlag(12201800) == std::optional<bool>{true});

    WriteValue<s32>(reinterpret_cast<u64>(nodes[1].data()), 0x28, 2);
    regs = {};
    regs.rdi = state;
    regs.rsi = save_ptr;
    regs.rdx = save.size();
    ObserveBbscNativeFlagProof(NativeSaveSerializeOffset, regs);
    regs = {};
    regs.rsp = stack_ptr;
    ObserveBbscNativeFlagProof(NativeEventFlagsSerializeBeginOffset, regs);
    WriteValue<s32>(reinterpret_cast<u64>(nodes[1].data()), 0x28, 1);
    assert(!bbsc_native_flag_proof); // Separate kind-2 storage cannot be tied to this flat span.

    regs = {};
    regs.rdi = state;
    regs.rsi = save_ptr;
    regs.rdx = save.size();
    ObserveBbscNativeFlagProof(NativeSaveSerializeOffset, regs);
    WriteValue<u32>(stack_ptr, 0x130, start);
    WriteValue<u32>(stack_ptr, 0xD4, start);
    regs = {};
    regs.rsp = stack_ptr;
    ObserveBbscNativeFlagProof(NativeEventFlagsSerializeBeginOffset, regs);
    assert(bbsc_native_flag_proof && bbsc_native_flag_proof->callback_started);
    WriteValue<u32>(reinterpret_cast<u64>(nodes[1].data()), 0x30, 16);
    WriteValue<u32>(stack_ptr, 0x130, start + span_bytes);
    regs.r13 = 1;
    ObserveBbscNativeFlagProof(NativeEventFlagsSerializeEndOffset, regs);
    assert(!bbsc_native_flag_proof); // A selected key-to-slot change across callback boundaries is unknown.
    WriteValue<u32>(reinterpret_cast<u64>(nodes[1].data()), 0x30, 0);
    mock_receipt_override_path = previous_path;
    mock_receipt_path_only = previous_path_only;
    mock_receipt_generation = previous_generation;
    mock_receipt_sha = previous_sha;
    if (previous_save_dir) setenv("BBSC_SAVE_DIR", previous_save_dir->c_str(), 1);
    else unsetenv("BBSC_SAVE_DIR");
    std::filesystem::remove_all(proof_dir, proof_ec);
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    assert(munmap(image, image_size) == 0);
    std::puts("PASS: production native flag observer binds kind-1 slots/span hash to a fresh exact save receipt");
}

static void TestNativeFlagCallbackHookCompatibility() {
    using namespace Core::Bloodborne;
    auto* page = static_cast<u8*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(page != MAP_FAILED);
    constexpr std::array<u8, 7> begin{
        0x49, 0x8B, 0x7D, 0x00, 0x48, 0x85, 0xFF}; // selected buffer read before 01399DE0
    constexpr std::array<u8, 7> end{
        0x8B, 0x84, 0x24, 0x30, 0x01, 0x00, 0x00}; // load post-callback cursor, at 0156AC94
    std::memcpy(page, begin.data(), begin.size());
    std::memcpy(page + 64, end.data(), end.size());
    assert(Core::InstallGuestCodeHook(page, begin, NativeEventFlagsSerializeBeginOffset,
                                      IgnoreGuestCodeHook));
    assert(Core::InstallGuestCodeHook(page + 64, end, NativeEventFlagsSerializeEndOffset,
                                      IgnoreGuestCodeHook));
    // The hook provider retains installed addresses for the process lifetime.
    std::puts("PASS: native event-flag callback hooks accept verified non-relative instruction spans");
}

// Captured 1.09 arithmetic, executed only against fixture memory through the real hook provider.
static void TestPartyInbox() {
    using namespace Core::Bloodborne;
    using namespace Core::Bbsc::Party;
    const Peer host{"host", 7};
    const std::array<Peer, 1> allowed{host};
    OnBbscPartyRoomState({1, 42, false, "host", {"host", "other"}});
    assert(GetBbscPartyRoomState() && GetBbscPartyRoomState()->owner == "host");
    assert(!SetBbscPartyPeers(std::array<Peer, 1>{Peer{"other", 8}}));
    assert(SetBbscPartyPeers(allowed));
    Message message; message.party[0] = 1; message.admission = 3;
    auto bytes = EncodeMessage(message);
    OnBbscPartyPacket({"other", 7}, bytes.data(), bytes.size());
    OnBbscPartyPacket({"host", 8}, bytes.data(), bytes.size());
    OnBbscPartyPacket(host, nullptr, bytes.size());
    assert(TakeBbscPartyPackets().empty());
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    std::fill(bytes.begin(), bytes.end(), 0);
    auto delivered = TakeBbscPartyPackets();
    assert(delivered.size() == 1 && delivered[0].peer == host && delivered[0].message == message);
    CapabilityMessage capability;
    capability.kind = CapabilityKind::Probe;
    capability.room = 42;
    capability.challenge[0] = 3;
    capability.own_proof[0] = 4;
    bytes = EncodeCapability(capability);
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    std::fill(bytes.begin(), bytes.end(), 0);
    auto capabilities = TakeBbscCapabilityPackets();
    assert(capabilities.size() == 1 && capabilities[0].peer == host);
    assert(DecodeCapability(capabilities[0].bytes) == capability);
    assert(TakeBbscCapabilityPackets().empty());
    for (unsigned seq = 0; seq < 1100; ++seq) {
        message.sequence = seq;
        bytes = EncodeMessage(message);
        OnBbscPartyPacket(host, bytes.data(), bytes.size());
    }
    assert(TakeBbscPartyPackets().size() == 1033);
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    assert(SetBbscPartyPeers({}) && TakeBbscPartyPackets().empty());
    assert(!SetBbscPartyPeers(std::array<Peer, 2>{host, host}));
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    assert(TakeBbscPartyPackets().empty());
    assert(SetBbscPartyPeers(allowed));
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    OnBbscPartyRoomState({1, 42, false, "other", {"other"}});
    OnBbscPartyPacket(host, bytes.data(), bytes.size());
    assert(TakeBbscPartyPackets().empty());
    OnBbscPartyRoomState({1, 0, false, {}, {}});
    assert(!GetBbscPartyRoomState() && !SetBbscPartyPeers(allowed));
    std::puts("PASS: party inbox pins room owner, copies/bounds/deduplicates messages and clears stale-room peers");
}

static void TestCapabilityAdmissionHostTick() {
    using namespace Core::Bloodborne;
    using namespace Core::Bbsc::Party;
    auto& established = Libraries::Np::NpSignaling::Helpers::bbsc_test_established_peers;
    auto& sent = Libraries::Np::NpSignaling::Helpers::bbsc_test_sent_packets;
    established = {{"guest", 41}};
    sent.clear();
    bbsc_test_capability_proofs = {};
    bbsc_test_capability_proofs.observation.own_sign.valid = true;
    bbsc_test_capability_proofs.observation.own_sign.user_id = 4127;
    bbsc_test_capability_proofs.observation.own_sign.character_id = 2723151752040083LL;
    bbsc_test_capability_proofs.observation.own_sign.summon_type = 0;
    bbsc_test_capability_proofs.observation.private_attempt = {
        Libraries::Http::BbscPrivateAttemptState::Ready, 1};
    bbsc_test_capability_proofs.observation.claim.valid = true;
    bbsc_test_capability_proofs.observation.claim.matched_unique_candidate = true;
    bbsc_test_capability_proofs.observation.claim.password_matches_own_sign = true;
    bbsc_test_capability_proofs.own.fill(0x11);
    bbsc_test_capability_proofs.selected.fill(0x22);
    bbsc_native_coop_hooks_ready = true;
    const char* prior_bbsc = std::getenv("BB_BBSC");
    const std::optional<std::string> prior_bbsc_value = prior_bbsc
        ? std::optional<std::string>{prior_bbsc} : std::nullopt;
    setenv("BB_BBSC", "1", 1);
    OnBbscPartyRoomState({2, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{Peer{"guest", 41}}));

    Capability probe;
    probe.kind = CapabilityKind::Probe;
    probe.room = 77;
    probe.challenge[0] = 9;
    probe.own_proof.fill(0x22);
    probe.selected_proof.fill(0x11);
    auto bytes = EncodeCapability(probe);
    ++bbsc_test_capability_proofs.observation.own_sign.character_id;
    OnBbscPartyPacket({"guest", 41}, bytes.data(), bytes.size());
    TickBbscAdmissionCoordinator();
    assert(sent.empty());
    --bbsc_test_capability_proofs.observation.own_sign.character_id;
    OnBbscPartyPacket({"guest", 41}, bytes.data(), bytes.size());
    TickBbscAdmissionCoordinator();
    assert(sent.size() == 1 && sent[0].first == (Peer{"guest", 41}));
    auto confirm = DecodeCapability(sent[0].second);
    assert(confirm && confirm->kind == CapabilityKind::Confirm && confirm->room == 77 &&
           confirm->challenge == probe.challenge && confirm->own_proof == bbsc_test_capability_proofs.own &&
           confirm->selected_proof == bbsc_test_capability_proofs.selected);

    sent.clear();
    probe.own_proof[0] ^= 1;
    bytes = EncodeCapability(probe);
    OnBbscPartyPacket({"guest", 41}, bytes.data(), bytes.size());
    TickBbscAdmissionCoordinator();
    assert(sent.empty());
    OnBbscPartyRoomState({2, 0, false, {}, {}});
    established.clear();
    bbsc_native_coop_hooks_ready = false;
    bbsc_test_capability_proofs = {};
    if (prior_bbsc_value) setenv("BB_BBSC", prior_bbsc_value->c_str(), 1);
    else unsetenv("BB_BBSC");
    std::puts("PASS: capability game tick confirms only the current actual member's matching claim proof");
}

static void TestPartyPeerSelectionRefresh() {
    using namespace Core::Bloodborne;
    using namespace Core::Bbsc::Party;
    auto& established = Libraries::Np::NpSignaling::Helpers::bbsc_test_established_peers;
    std::this_thread::sleep_for(std::chrono::milliseconds(510)); // Wait out the production refresh throttle.
    OnBbscPartyRoomState({2, 77, false, "host", {"host", "guest"}});
    established = {{"host", 19}, {"guest", 21}, {"unrelated", 25}};
    TickBbscPartyPeerSelection();
    Message message;
    message.party[0] = 5;
    message.admission = 4;
    auto bytes = EncodeMessage(message);
    OnBbscPartyPacket({"guest", 21}, bytes.data(), bytes.size());
    OnBbscPartyPacket({"host", 19}, bytes.data(), bytes.size());
    auto received = TakeBbscPartyPackets();
    assert(received.size() == 1 && received[0].peer.online_id == "host");

    OnBbscPartyRoomState({2, 77, false, "host", {"host", "guest"}});
    established = {{"host", 20}, {"host", 22}}; // Multiple connections are ambiguous.
    TickBbscPartyPeerSelection();
    OnBbscPartyPacket({"host", 20}, bytes.data(), bytes.size());
    assert(TakeBbscPartyPackets().empty());
    OnBbscPartyRoomState({2, 0, false, {}, {}});
    established.clear();
}

static void TestNativeCycleHookReplay() {
    using namespace Core::Bloodborne;
    std::vector<u8> image(0x05600000);
    std::array<u8, 0x1800> global{};
    std::array<u8, 0x100> slots{}, event{};
    std::array<u8, 8> command{};
    const u64 state = reinterpret_cast<u64>(global.data());
    const u64 manager = reinterpret_cast<u64>(slots.data());
    const u64 task = reinterpret_cast<u64>(event.data());
    const u64 instruction = reinterpret_cast<u64>(command.data());
    image_base = reinterpret_cast<u64>(image.data());
    MemoryPatcher::g_eboot_image_size = image.size();
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, manager);
    WriteValue<s32>(state, 0x1490, 1);
    WriteValue<u8>(state, 0x1592, 1);
    WriteValue<u64>(task, 0xB0, instruction);
    WriteValue<u32>(instruction, 4, 21);
    auto* page = static_cast<u8*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(page != MAP_FAILED);
    const std::vector<u8> engine_math{
        0x8B, 0x71, 0x68, 0x31, 0xD2, 0x89, 0xF7, 0xFF, 0xC7, 0x89, 0x79, 0x68,
        0x48, 0x8D, 0x49, 0x68, 0xC7, 0x45, 0xF4, 0x07, 0x00, 0x00, 0x00, 0x78,
        0x0D, 0x83, 0xFE, 0x06, 0x48, 0x8D, 0x55, 0xF4, 0x48, 0x0F, 0x4E, 0xD1,
        0x8B, 0x12, 0x89, 0x11};
    const std::vector<u8> event_math{
        0x8B, 0x50, 0x68, 0x31, 0xC9, 0x89, 0xD6, 0xFF, 0xC6, 0x89, 0x70, 0x68,
        0x48, 0x8D, 0x40, 0x68, 0xC7, 0x45, 0x90, 0x07, 0x00, 0x00, 0x00, 0x78,
        0x0D, 0x83, 0xFA, 0x06, 0x48, 0x8D, 0x4D, 0x90, 0x48, 0x0F, 0x4E, 0xC8,
        0x8B, 0x09, 0x89, 0x08, 0x41, 0xB6, 0x01};
    static std::optional<BbscNativeSaveEvidence> observed;
    static unsigned callbacks;
    const auto callback = +[](u64 offset, const Core::GuestRegisterSnapshot* regs) {
        ++callbacks;
        observed = ReadBbscNativeSaveEvidence(offset, *regs);
    };
    const u64 offsets[]{0x0131D2A8, 0x01334C05, 0x017C045A};
    for (unsigned i = 0; i < 3; ++i) {
        const bool event_site = i == 2;
        std::vector<u8> code = event_site ? std::vector<u8>{
            0x55, 0x48, 0x89, 0xE5, 0x48, 0x81, 0xEC, 0x80, 0x00, 0x00, 0x00,
            0x53, 0x41, 0x55, 0x41, 0x56, 0x48, 0x89, 0xF8, 0x48, 0x89, 0xF3,
            0x49, 0x89, 0xD5, 0x4D, 0x8B, 0x55, 0x00, 0x4C, 0x89, 0x55, 0xD0}
            : std::vector<u8>{
            0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x89, 0xF9,
            0x48, 0x89, 0xF0, 0x4C, 0x8B, 0x00, 0x4C, 0x89, 0x45, 0xF8};
        std::size_t guard = 0;
        if (i == 1) {
            code.insert(code.end(), {0x85, 0xD2, 0x0F, 0x84, 0, 0, 0, 0});
            guard = code.size() - 4;
        }
        const auto& math = event_site ? event_math : engine_math;
        code.insert(code.end(), math.begin(), math.end());
        const std::size_t site_at = code.size();
        if (guard) {
            const s32 displacement = static_cast<s32>(site_at - (guard + 4));
            std::memcpy(code.data() + guard, &displacement, sizeof(displacement));
        }
        const auto site = std::find_if(Sites.begin(), Sites.end(),
                                      [&](const TraceSite& value) { return value.offset == offsets[i]; });
        assert(site != Sites.end());
        code.insert(code.end(), site->prologue.begin(), site->prologue.begin() + site->prologue_size);
        if (event_site) code.insert(code.end(), {0x41, 0x5E, 0x41, 0x5D, 0x5B});
        code.insert(code.end(), {0xC9, 0xC3});
        auto* entry = page + i * 512;
        assert(!mprotect(page, 4096, PROT_READ | PROT_WRITE | PROT_EXEC));
        std::memcpy(entry, code.data(), code.size());
        assert(Core::InstallGuestCodeHook(entry + site_at,
               std::span<const u8>(site->prologue.data(), site->prologue_size), offsets[i], callback));
        using Function = void (*)(void*, void*, u64);
        const auto run = reinterpret_cast<Function>(entry);
        u64 canary = 0x12345678;
        for (s32 previous : {0, 6, 7}) {
            WriteValue<s32>(manager, 0x68, previous);
            callbacks = 0; observed.reset();
            run(slots.data(), event_site ? event.data() : static_cast<void*>(&canary),
                event_site ? reinterpret_cast<u64>(&canary) : 1);
            assert(callbacks == 1 && observed && observed->stage == "cycle-increment-applied");
            assert(observed->previous_capped_cycle == previous &&
                   observed->live_capped_cycle == std::min(previous + 1, 7));
            assert(ReadValue<s32>(manager, 0x68) == std::min(previous + 1, 7));
        }
        if (i == 1) {
            WriteValue<s32>(manager, 0x68, 7);
            callbacks = 0; observed.reset();
            run(slots.data(), &canary, 0);
            assert(callbacks == 1 && !observed && ReadValue<s32>(manager, 0x68) == 7);
        }
    }
    assert(!munmap(page, 4096));
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    std::puts("PASS: actual hook trampolines replay captured increment arithmetic including 7-to-7 and skipped guard");
}

static void TestManagedCycleHookCoverage() {
    using namespace Core::Bloodborne;
    const u64 required[]{NativeSaveSerializeOffset, NativeSaveSerializeEndOffset,
        NativeCycleIncrementOffset, NativeCycleLuaIntentOffset, NativeCycleEventIncrementOffset,
        NativeCycleAppliedOffset, NativeCycleLuaAppliedOffset, NativeCycleEventAppliedOffset};
    for (const auto offset : required) {
        const auto site = std::find_if(Sites.begin(), Sites.end(),
            [=](const TraceSite& value) { return value.offset == offset; });
        assert(site != Sites.end() && ShouldInstallBbscTraceSite(*site, false));
    }
    const auto skipped_lua_entry = std::find_if(Sites.begin(), Sites.end(),
        [](const TraceSite& value) { return value.offset == NativeCycleLuaIncrementOffset; });
    assert(skipped_lua_entry != Sites.end() && !ShouldInstallBbscTraceSite(*skipped_lua_entry, false));
    const auto ordinary_save = std::find_if(Sites.begin(), Sites.end(),
        [](const TraceSite& value) { return value.offset == NativeSaveLoadOffset; });
    assert(ordinary_save != Sites.end() && !ShouldInstallBbscTraceSite(*ordinary_save, false));
    const bool old_installed = bbsc_managed_recovery_hooks_installed;
    bbsc_managed_recovery_hooks_installed = true;
    for (const auto offset : required) {
        const auto site = std::find_if(Sites.begin(), Sites.end(),
            [=](const TraceSite& value) { return value.offset == offset; });
        assert(site != Sites.end() && IsManagedRecoveryProvidedTraceSite(*site));
    }
    bbsc_managed_recovery_hooks_installed = old_installed;
    std::puts("PASS: managed cycle/receipt hooks remain installed with identity diagnostics disabled and are shared with full tracing");
}

static void TestNativeCycleAppliedEvidence() {
    using namespace Core::Bloodborne;
    std::vector<u8> image(0x05600000);
    std::array<u8, 0x1800> global{};
    std::array<u8, 0x100> slots{}, event{};
    std::array<u8, 8> command{};
    image_base = reinterpret_cast<u64>(image.data());
    MemoryPatcher::g_eboot_image_size = image.size();
    const u64 state = reinterpret_cast<u64>(global.data());
    const u64 manager = reinterpret_cast<u64>(slots.data());
    const u64 task = reinterpret_cast<u64>(event.data());
    const u64 instruction = reinterpret_cast<u64>(command.data());
    WriteValue<u64>(image_base, GlobalStatePointerOffset, state);
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, manager);
    WriteValue<s32>(state, 0x1490, 1);
    WriteValue<u8>(state, 0x1592, 1);
    WriteValue<u64>(task, 0xB0, instruction);
    WriteValue<u32>(instruction, 4, 21);
    for (const u64 offset : {u64{0x0131D2A8}, u64{0x01334C05}, u64{0x017C045A}}) {
        const auto site = std::find_if(Sites.begin(), Sites.end(),
                                      [=](const TraceSite& value) { return value.offset == offset; });
        assert(site != Sites.end() && site->kind == TraceKind::SaveLifecycle);
        const bool event_site = offset == 0x017C045A;
        for (s32 previous : {0, 6, 7}) {
            const s32 next = std::min(previous + 1, 7);
            WriteValue<s32>(manager, 0x68, next);
            Core::GuestRegisterSnapshot regs{};
            regs.rbx = task;
            regs.rax = event_site ? manager + 0x68 : 0;
            regs.rcx = event_site ? next : manager + 0x68;
            regs.rdx = event_site ? previous : next;
            regs.rsi = event_site ? previous + 1 : previous;
            regs.rdi = previous + 1;
            regs.r14 = 1;
            const auto before = slots;
            const auto evidence = ReadBbscNativeSaveEvidence(offset, regs);
            assert(evidence && evidence->stage == "cycle-increment-applied" &&
                   evidence->live_capped_cycle == next && evidence->slot == 1 &&
                   evidence->own_world && evidence->success == 1 &&
                   evidence->previous_capped_cycle == previous);
            assert(!evidence->snapshot && evidence->buffer == 0 && slots == before);
            // The guard/other event must never masquerade as an applied increment.
            auto invalid = regs;
            invalid.rcx = event_site ? 99 : manager;
            assert(!ReadBbscNativeSaveEvidence(offset, invalid));
            invalid = regs; invalid.rsi = 99;
            assert(!ReadBbscNativeSaveEvidence(offset, invalid));
            if (event_site) {
                for (u32 other : {0, 20, 22}) {
                    WriteValue<u32>(instruction, 4, other);
                    assert(!ReadBbscNativeSaveEvidence(offset, regs));
                }
                WriteValue<u32>(instruction, 4, 21);
                invalid = regs; invalid.r14 = 0;
                assert(!ReadBbscNativeSaveEvidence(offset, invalid));
                invalid = regs; invalid.rbx = 0;
                assert(!ReadBbscNativeSaveEvidence(offset, invalid));
                invalid = regs; invalid.rax = manager;
                assert(!ReadBbscNativeSaveEvidence(offset, invalid));
            } else {
                invalid = regs; invalid.rdi = 99;
                assert(!ReadBbscNativeSaveEvidence(offset, invalid));
            }
            WriteValue<s32>(manager, 0x68, next == 7 ? 6 : 0);
            assert(!ReadBbscNativeSaveEvidence(offset, regs));
        }
    }
    image_base = 0;
    MemoryPatcher::g_eboot_image_size = 0;
    std::puts("PASS: post-increment evidence rejects skipped Lua guard/other commands and observes 7-to-7");
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
    for (const u64 offset : {u64{0x0131D260}, u64{0x01334BB0}, NativeCycleLuaIntentOffset,
                             u64{0x017C0FD2}}) {
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

static bool native_lamp_available = true;
static u32 native_respawn_map = 0x18010000;
static u32* PS4_SYSV_ABI TestNativeRespawnMap(u32* output, s32 point) {
    assert(point == 2412950);
    *output = native_lamp_available ? native_respawn_map : 0xFFFFFFFF;
    return output;
}
static u8 PS4_SYSV_ABI TestNativeHomePoint(u64, float* position, float* rotation, s32 point) {
    assert(point == 2412950);
    const std::array<float, 4> pos{100, 200, 300, 1}, rot{0, 0.75f, 0, 0};
    std::copy(pos.begin(), pos.end(), position);
    std::copy(rot.begin(), rot.end(), rotation);
    return native_lamp_available ? 1 : 0;
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
    std::array<u8, 0x40> flag_manager{};
    std::array<u8, 0x20> flag_head{};
    std::array<u8, 0x40> flag_node{};
    std::array<u8, BbscWorldGroupBytes> live_flags{};
    std::array<u8, 0x130> matching{};
    std::array<u8, 0x80> local_world_state{};
    std::array<u8, 0x28> local_modules{};
    std::array<u8, 0x100> local_module_data{};
    const auto address = [](auto& data) { return reinterpret_cast<u64>(data.data()); };
    image_base = address(image);
    MemoryPatcher::g_eboot_image_size = image.size();
    WriteValue<u64>(image_base, GlobalStatePointerOffset, address(global));
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, address(slots));
    WriteValue<u64>(image_base, MatchingStatePointerOffset, address(matching));
    WriteValue<u64>(image_base, CandidateLocalStatePointerOffset, address(local_world_state));
    WriteValue<u64>(address(slots), 8, address(local));
    WriteValue<u64>(address(local_world_state), 0x60, address(local));
    WriteValue<u64>(address(local), 0x3B0, address(local_modules));
    WriteValue<u64>(address(local_modules), 0x20, address(local_module_data));
    WriteValue<s32>(address(local_module_data), 0xF8, 1000);
    WriteValue<s32>(address(global), 0x1490, 1);
    WriteValue<u8>(address(global), 0x1592, 1);
    WriteValue<s32>(address(matching), 0x124, 0);
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
    auto key = ReadBbscPartyCharacterKey();
    assert(key && key->save_dir == fs::canonical(folder).string());
    assert(key->user == "4127" && key->slot == 1 && key->cycle == 0);
    assert((key->character == Core::Bbsc::Party::PartyId{
        0x1f, 0x10, 0, 0, 0, 0, 0, 0, 0x93, 0x8e, 0xd9, 0x2c, 0xb1, 0xac, 0x09, 0}));
    const auto original_key = *key;
    // Normal admission remains readiness-gated. The one explicit loading-boundary read used by
    // the verified respawn resolver still binds the exact selected native save and IDs.
    WriteValue<u8>(address(global), 0x08, 1);
    assert(!ReadBbscPartyCharacterKey());
    assert(ReadBbscPartyCharacterKey(true) == original_key);
    WriteValue<u8>(address(global), 0x08, 0);
    // With no launcher-provided BBSC_SAVE_DIR, recovery can bind only from the runtime's
    // selected-slot receipt; it must not scan for a matching-looking profile folder.
    mock_receipt_override_path = file.string();
    mock_receipt_path_only = true;
    unsetenv("BBSC_SAVE_DIR");
    const auto receipt_bound_key = ReadBbscPartyCharacterKey();
    assert(receipt_bound_key && *receipt_bound_key == original_key);
    setenv("BBSC_SAVE_DIR", folder.c_str(), 1);
    mock_receipt_path_only = false;
    mock_receipt_override_path.clear();
    WriteValue<s32>(address(slots), 0x68, 1);
    assert(!ReadBbscPartyCharacterKey()); // An unsaved cycle change cannot reuse the old journal.
    WriteValue<s32>(address(slots), 0x68, 0);
    WriteValue<u64>(address(local), 0x690, 2723151752040084ULL);
    assert(!ReadBbscPartyCharacterKey()); // Do not adopt the deleted slot's old payload.
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040084ULL);
    save();
    key = ReadBbscPartyCharacterKey();
    assert(key && key->character != original_key.character);
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040083ULL);
    save();
    // Native origin and the pre-party native respawn point are captured together.
    std::array<u8, 0xA90> root{};
    std::array<u8, 0x30> map_list{}, map_array{};
    std::array<u8, 0xA0> map_entry{};
    std::array<u8, 0x70> local_state{}, model{};
    std::array<u8, 0x500> local_world{};
    std::array<u8, 0x40> player_modules{};
    std::array<u8, 0x110> player_data{};
    std::array<u8, 0x10> character{};
    std::array<u8, 0x3C0> character_state{};
    std::array<u8, 0x200> transform{};
    WriteValue<u64>(image_base, SummonManagerRootPointerOffset, address(root));
    WriteValue<s32>(address(root), 0xA80, 241010);
    WriteValue<u64>(image_base, CurrentMapListPointerOffset, address(map_list));
    WriteValue<u64>(address(map_list), 0x10, address(map_array));
    WriteValue<s32>(address(map_array), 0x18, 1);
    WriteValue<u64>(address(map_array), 0x20, address(map_entry));
    WriteValue<u32>(address(map_entry), 8, 0x18010000);
    WriteValue<u64>(image_base, CandidateLocalStatePointerOffset, address(local_state));
    WriteValue<u64>(address(local_state), 0x60, address(local_world));
    WriteValue<u64>(address(local_world), 0x3B0, address(player_modules));
    WriteValue<u64>(address(player_modules), 0x20, address(player_data));
    WriteValue<s32>(address(player_data), 0xF8, 100);
    WriteValue<u64>(address(local_world), 0x58, address(character));
    WriteValue<u64>(address(character), 8, address(character_state));
    WriteValue<u64>(address(character_state), 0x3B0, address(model));
    WriteValue<u64>(address(model), 0x68, address(transform));
    WriteValue<float>(address(transform), 0x1D4, .5f);
    WriteValue<float>(address(transform), 0x1E0, 10);
    WriteValue<float>(address(transform), 0x1E4, 20);
    WriteValue<float>(address(transform), 0x1E8, 30);
    WriteValue<s32>(address(global), 0x1528, 2412950);
    bbsc_own_world.clear();
    bbsc_own_world_key.reset();
    bbsc_own_world_slot.reset();
    bbsc_own_world_dirty = false;
    std::array<u8, BbscWorldGroupBytes> saved_group{};
    saved_group[0] = 0x5A;
    saved_group.back() = 0xA5;
    // No flag manager or captured groups means there is no complete home snapshot.
    CaptureBbscOwnWorldBeforeJoin();
    assert(bbsc_own_world.empty() && !bbsc_own_world_key);
    assert(!SerializeBbscOwnWorldSnapshot());
    // A ready native map table supplies the actual capture group at the join seam.
    std::fill(live_flags.begin(), live_flags.end(), 0xCC);
    WriteValue<u64>(image_base, WorldFlagManPointerOffset, address(flag_manager));
    WriteValue<u32>(address(flag_manager), 0x1C, 1000);
    WriteValue<u32>(address(flag_manager), 0x20, BbscWorldGroupBytes);
    WriteValue<u32>(address(flag_manager), 0x24, 1);
    WriteValue<u64>(address(flag_manager), 0x38, address(flag_head));
    WriteValue<u8>(address(flag_head), 0x19, 1);
    WriteValue<u64>(address(flag_head), 0x08, address(flag_head));
    CaptureBbscOwnWorldBeforeJoin();
    assert(bbsc_own_world.empty() && !bbsc_own_world_key); // A ready but empty table is not a snapshot.
    WriteValue<u64>(address(flag_head), 0x08, address(flag_node));
    WriteValue<u64>(address(flag_node), 0x00, address(flag_head));
    WriteValue<u64>(address(flag_node), 0x10, address(flag_head));
    WriteValue<u8>(address(flag_node), 0x19, 0);
    WriteValue<u32>(address(flag_node), 0x20, 12410); // kind1, area24, block1
    WriteValue<s32>(address(flag_node), 0x28, 2);
    WriteValue<u64>(address(flag_node), 0x30, address(live_flags));
    std::copy(saved_group.begin(), saved_group.end(), live_flags.begin());
    CaptureBbscOwnWorldBeforeJoin();
    assert(bbsc_own_world.size() == 1 && bbsc_own_world.at(12410) == saved_group);
    assert(bbsc_own_world_key == original_key);
    assert(SerializeBbscOwnWorldSnapshot());
    std::fill(live_flags.begin(), live_flags.end(), 0xFF); // guest-world state to be overwritten
    const u64 resolver = image_base + 0x0154AFC0, page = resolver & ~u64{4095};
    assert(!mprotect(reinterpret_cast<void*>(page), 4096, PROT_READ | PROT_WRITE | PROT_EXEC));
    const u64 map_resolver = image_base + 0x013CA100, map_page = map_resolver & ~u64{4095};
    assert(!mprotect(reinterpret_cast<void*>(map_page), 4096, PROT_READ | PROT_WRITE | PROT_EXEC));
    const std::array<u8, 12> jump{0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
    std::memcpy(reinterpret_cast<void*>(resolver), jump.data(), jump.size());
    WriteValue<u64>(resolver, 2, reinterpret_cast<u64>(TestNativeHomePoint));
    std::memcpy(reinterpret_cast<void*>(map_resolver), jump.data(), jump.size());
    WriteValue<u64>(map_resolver, 2, reinterpret_cast<u64>(TestNativeRespawnMap));
    std::error_code ec;
    bbsc_own_world_key->character[0] ^= 1;
    assert(!PrepareBbscNativePartyHome({4, 6}, ec) && ec == std::errc::invalid_argument);
    bbsc_own_world_key->character[0] ^= 1;
    const auto good_receipt_hash = mock_receipt_sha;
    mock_receipt_sha.assign(64, '0');
    assert(!PrepareBbscNativePartyHome({4, 6}, ec, true) && ec);
    assert(!Core::Bbsc::Party::LoadJournal(BbscPartyHomePath(original_key), original_key, ec));
    mock_receipt_sha = good_receipt_hash;

    // The guest has no outgoing claim; the host selects its own advertised sign by proof.
    using namespace Core::Bbsc::Party;
    bbsc_test_admission_tickets.clear();
    bbsc_test_admission_resolutions.clear();
    auto& admission_peers = Libraries::Np::NpSignaling::Helpers::bbsc_test_established_peers;
    auto& admission_packets = Libraries::Np::NpSignaling::Helpers::bbsc_test_sent_packets;
    admission_peers = {{"host", 41}};
    admission_packets.clear();
    bbsc_test_capability_proofs = {};
    bbsc_test_capability_proofs.observation.private_attempt = {
        Libraries::Http::BbscPrivateAttemptState::Ready, 12};
    bbsc_test_capability_proofs.observation.own_sign = {
        .valid = true, .session_id = "guest-session", .user_id = 4127,
        .character_id = 2723151752040083LL, .summon_type = 0};
    bbsc_test_capability_proofs.own.fill(0x11);
    assert(ProofIsZero(bbsc_test_capability_proofs.selected));
    bbsc_native_coop_hooks_ready = true;
    const char* bbsc_setting = std::getenv("BB_BBSC");
    const std::optional<std::string> previous_bbsc_setting = bbsc_setting
        ? std::optional<std::string>{bbsc_setting} : std::nullopt;
    setenv("BB_BBSC", "1", 1);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    Libraries::Np::NpMatching2::BbscAdmissionTicket ticket;
    ticket.context = 34;
    ticket.incarnation = 2;
    ticket.request = 5;
    ticket.room = 77;
    ticket.nonce = 0x99887766;
    ticket.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    std::strcpy(ticket.owner.data, "host");
    bbsc_test_admission_tickets = {ticket};
    TickBbscAdmissionCoordinator();
    assert(BbscAdmissionSuppressesOwnWorldRecovery(original_key));
    auto another_character = original_key;
    another_character.character[0] ^= 1;
    assert(!BbscAdmissionSuppressesOwnWorldRecovery(another_character));
    assert(admission_packets.size() == 1 && admission_packets[0].first == (Peer{"host", 41}));
    const auto probe = DecodeCapability(admission_packets[0].second);
    assert(probe && probe->kind == CapabilityKind::Probe && probe->room == ticket.room &&
           probe->own_proof == bbsc_test_capability_proofs.own &&
           ProofIsZero(probe->selected_proof));
    Capability confirm;
    confirm.kind = CapabilityKind::Confirm;
    confirm.room = ticket.room;
    confirm.challenge = probe->challenge;
    confirm.own_proof.fill(0x33);
    confirm.selected_proof = bbsc_test_capability_proofs.own;
    auto confirm_bytes = EncodeCapability(confirm);
    OnBbscPartyPacket({"unrelated", 99}, confirm_bytes.data(), confirm_bytes.size());
    TickBbscAdmissionCoordinator();
    assert(bbsc_test_admission_resolutions.empty());
    confirm.challenge[0] ^= 1;
    confirm_bytes = EncodeCapability(confirm);
    OnBbscPartyPacket({"host", 41}, confirm_bytes.data(), confirm_bytes.size());
    TickBbscAdmissionCoordinator();
    assert(bbsc_test_admission_resolutions.empty());
    confirm.challenge = probe->challenge;
    confirm.selected_proof.fill(0x99);
    confirm_bytes = EncodeCapability(confirm);
    OnBbscPartyPacket({"host", 41}, confirm_bytes.data(), confirm_bytes.size());
    TickBbscAdmissionCoordinator();
    assert(bbsc_test_admission_resolutions.empty());
    confirm.selected_proof = bbsc_test_capability_proofs.own;
    confirm_bytes = EncodeCapability(confirm);
    OnBbscPartyPacket({"host", 41}, confirm_bytes.data(), confirm_bytes.size());
    TickBbscAdmissionCoordinator();
    const auto admission_record = Core::Bbsc::Party::LoadJournal(
        BbscPartyHomePath(original_key), original_key, ec);
    assert(admission_record && admission_record->phase == Phase::Visiting &&
           admission_record->admission == ticket.nonce && bbsc_test_admission_resolutions.size() == 1 &&
           bbsc_test_admission_resolutions[0].second);
    const auto selected_backup = BbscPartyHomePath(original_key).parent_path() /
        (BbscPartyHomePath(original_key).stem().string() + ".selected-save-" +
         std::to_string(mock_receipt_generation) + "-" + mock_receipt_sha + ".bin");
    assert(std::filesystem::is_regular_file(selected_backup));
    // A queued callback has a durable Visiting record before the native Lua event, and must not
    // be mistaken for a crashed return while the join load is still pending.
    assert(BbscAdmissionSuppressesOwnWorldRecovery(original_key));
    assert(ConsumeBbscAdmittedJoinForNativeEvent());
    assert(bbsc_admitted_join && bbsc_admitted_join->consumed);
    assert(BbscAdmissionSuppressesOwnWorldRecovery(original_key));
    WriteValue<s32>(address(matching), 0x124, 6);
    WriteValue<s32>(address(local), 0xA4, 1);
    WriteValue<u8>(address(global), 0x08, 0);

    // A production game-tick exchange proves the first-playable boundary, durable Ack, and
    // post-boundary event path using the real temporary journal and the existing peer inbox.
    const auto previous_delivery_flag_manager = flag_manager;
    std::array<u8, 0x20> delivery_head{};
    std::vector<std::array<u8, 0x40>> delivery_nodes(BbscNativeProofGroupKeyCount);
    std::vector<u8> delivery_storage(BbscNativeProofGroupKeyCount * BbscWorldGroupBytes);
    const u64 delivery_head_addr = address(delivery_head);
    WriteValue<u8>(delivery_head_addr, 0x19, 1);
    const auto build_delivery_tree = [&](auto&& self, size_t first, size_t last, u64 parent) -> u64 {
        if (first >= last) return delivery_head_addr;
        const size_t middle = first + (last - first) / 2;
        const u64 node = address(delivery_nodes[middle]);
        WriteValue<u64>(node, 0x08, parent);
        WriteValue<u8>(node, 0x19, 0);
        WriteValue<u32>(node, 0x20, BbscNativeProofGroupKeys[middle]);
        WriteValue<s32>(node, 0x28, 1);
        WriteValue<u32>(node, 0x30, static_cast<u32>(middle));
        WriteValue<u64>(node, 0x00, self(self, first, middle, node));
        WriteValue<u64>(node, 0x10, self(self, middle + 1, last, node));
        return node;
    };
    WriteValue<u64>(delivery_head_addr, 0x08,
        build_delivery_tree(build_delivery_tree, 0, delivery_nodes.size(), delivery_head_addr));
    WriteValue<u32>(address(flag_manager), 0x1C, 1000);
    WriteValue<u32>(address(flag_manager), 0x20, BbscWorldGroupBytes);
    WriteValue<u32>(address(flag_manager), 0x24, static_cast<u32>(delivery_nodes.size()));
    WriteValue<u64>(address(flag_manager), 0x28, address(delivery_storage));
    WriteValue<u64>(address(flag_manager), 0x38, delivery_head_addr);
    WriteValue<u64>(image_base, WorldFlagManPointerOffset, address(flag_manager));
    const auto set_catalog_flag = [&](u32 flag) {
        const auto group = std::ranges::find(BbscNativeProofGroupKeys, flag / 1000);
        assert(group != BbscNativeProofGroupKeys.end());
        const size_t slot = static_cast<size_t>(group - BbscNativeProofGroupKeys.begin());
        const u32 bit = flag % 1000;
        delivery_storage[slot * BbscWorldGroupBytes + bit / 8] |=
            static_cast<u8>(0x80u >> (bit % 8));
    };
    set_catalog_flag(12101800); // Pre-existing host completion belongs in baseline only.
    const auto catalog_before = ReadBbscPartyCatalogState();
    assert(catalog_before && (*catalog_before)[0]); // Native flag bytes are MSB-first.
    auto& established = Libraries::Np::NpSignaling::Helpers::bbsc_test_established_peers;
    auto& sent_packets = Libraries::Np::NpSignaling::Helpers::bbsc_test_sent_packets;
    established = {{"guest", 42}, {"host", 41}};
    sent_packets.clear();
    WriteValue<s32>(address(local), 0xA4, 0); // Host role, first playable frame.
    bbsc_test_capability_proofs = {};
    bbsc_test_capability_proofs.observation.private_attempt = {
        Libraries::Http::BbscPrivateAttemptState::Ready, 22};
    bbsc_test_capability_proofs.observation.own_sign = {
        .valid = true, .session_id = "host-session", .user_id = 4127,
        .character_id = 2723151752040083LL, .summon_type = 0};
    bbsc_test_capability_proofs.observation.claim.valid = true;
    bbsc_test_capability_proofs.observation.claim.matched_unique_candidate = true;
    bbsc_test_capability_proofs.observation.claim.password_matches_own_sign = true;
    bbsc_test_capability_proofs.own.fill(0x33);
    bbsc_test_capability_proofs.selected.fill(0x11);
    OnBbscPartyRoomState({34, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"guest", 42}}}));
    Capability host_probe;
    host_probe.kind = CapabilityKind::Probe;
    host_probe.room = 77;
    host_probe.challenge = probe->challenge;
    host_probe.own_proof.fill(0x11);
    auto host_probe_bytes = EncodeCapability(host_probe);
    OnBbscPartyPacket({"guest", 42}, host_probe_bytes.data(), host_probe_bytes.size());
    TickBbscAdmissionCoordinator(); // Host capability confirmation stamps the exact peer/challenge.
    assert(std::ranges::any_of(sent_packets, [](const auto& packet) {
        const auto decoded = DecodeCapability(packet.second);
        return decoded && decoded->kind == CapabilityKind::Confirm;
    }));
    sent_packets.clear();

    WriteValue<s32>(address(local), 0xA4, 1); // Guest becomes playable; this is when it requests baseline.
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));
    const auto guest_room = GetBbscPartyRoomState();
    const auto guest_live_key = ReadBbscPartyCharacterKey(false);
    assert(bbsc_admitted_join && bbsc_admitted_join->consumed && guest_room && !guest_live_key &&
           BbscPartyGuestPlayable(*guest_room, bbsc_admitted_join->key));
    TickBbscPartyEventDelivery();
    assert(sent_packets.size() == 1 && sent_packets[0].first == (Peer{"host", 41}));
    const auto guest_hello = sent_packets.front().second;
    sent_packets.clear();

    WriteValue<s32>(address(local), 0xA4, 0); // Host captures its complete baseline on this game tick.
    OnBbscPartyRoomState({34, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"guest", 42}}}));
    OnBbscPartyPacket({"guest", 42}, guest_hello.data(), guest_hello.size());
    TickBbscPartyEventDelivery();
    assert(!sent_packets.empty());
    set_catalog_flag(12201800); // Earned after the boundary, while baseline packets are still in flight.
    assert(!bbsc_confirmed_party_peers.empty());
    bbsc_confirmed_party_peers.front().last_send_ms = SteadyNowMs() - 1'000;
    TickBbscPartyEventDelivery(); // Baseline and event remain retryable until durable Ack.
    auto host_delivery = sent_packets;
    sent_packets.clear();

    WriteValue<s32>(address(local), 0xA4, 1);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));
    for (const auto& packet : host_delivery)
        OnBbscPartyPacket({"host", 41}, packet.second.data(), packet.second.size());
    auto blocked_lease = CharacterLease::Acquire(BbscPartyHomePath(original_key), ec);
    assert(blocked_lease); // A journal lease failure must hold the Ack.
    TickBbscPartyEventDelivery();
    assert(bbsc_guest_party_admission && bbsc_guest_party_admission->receiver.Active() &&
           !bbsc_guest_party_admission->durable_baseline_ack);
    assert(std::ranges::none_of(sent_packets, [](const auto& packet) {
        const auto decoded = DecodeMessage(packet.second);
        return decoded && decoded->kind == Kind::Ack;
    }));
    blocked_lease.reset();
    TickBbscPartyEventDelivery();
    assert(bbsc_guest_party_admission->durable_baseline_ack);
    const auto after_baseline = LoadJournal(BbscPartyHomePath(original_key), original_key, ec);
    assert(after_baseline && after_baseline->committed.empty() &&
           std::ranges::find(after_baseline->pending, 12101800) == after_baseline->pending.end() &&
           std::ranges::find(after_baseline->pending, 12201800) != after_baseline->pending.end());
    assert(BbscAdmissionSuppressesOwnWorldRecovery(original_key) == false);
    assert(!bbsc_admitted_join); // The recovery bypass ends only after durable baseline Ack.
    auto guest_ack_packets = sent_packets;
    sent_packets.clear();

    WriteValue<s32>(address(local), 0xA4, 0);
    OnBbscPartyRoomState({34, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"guest", 42}}}));
    for (const auto& packet : guest_ack_packets)
        OnBbscPartyPacket({"guest", 42}, packet.second.data(), packet.second.size());
    TickBbscPartyEventDelivery(); // Durable Ack retires setup and the post-boundary event.
    sent_packets.clear();
    TickBbscPartyEventDelivery();
    auto first_event_packets = sent_packets;
    assert(std::ranges::none_of(first_event_packets, [](const auto& packet) {
        const auto decoded = DecodeMessage(packet.second);
        return decoded && decoded->kind == Kind::Event && decoded->events == std::vector<u32>{12201800};
    }));
    sent_packets.clear();

    WriteValue<s32>(address(local), 0xA4, 1);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));
    for (const auto& packet : first_event_packets)
        OnBbscPartyPacket({"host", 41}, packet.second.data(), packet.second.size());
    Core::Bbsc::Party::Message wrong_peer_event;
    wrong_peer_event.party = probe->challenge;
    wrong_peer_event.admission = ticket.nonce;
    wrong_peer_event.sequence = 2;
    wrong_peer_event.kind = Kind::Event;
    wrong_peer_event.events = {12301800};
    const auto wrong_peer_bytes = EncodeMessage(wrong_peer_event);
    OnBbscPartyPacket({"stranger", 99}, wrong_peer_bytes.data(), wrong_peer_bytes.size());
    TickBbscPartyEventDelivery();
    auto earned_after_first = LoadJournal(BbscPartyHomePath(original_key), original_key, ec);
    assert(earned_after_first && earned_after_first->pending == std::vector<u32>{12201800});
    sent_packets.clear();

    // Death does not close a same-party channel. Host flags continue to be observed with the
    // native world available even when host HP is zero.
    WriteValue<s32>(address(local), 0xA4, 0);
    WriteValue<s32>(address(local_module_data), 0xF8, 0);
    set_catalog_flag(12301800);
    OnBbscPartyRoomState({34, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"guest", 42}}}));
    bbsc_confirmed_party_peers.front().last_send_ms = SteadyNowMs() - 1'000;
    TickBbscPartyEventDelivery();
    auto death_event_packets = sent_packets;
    assert(std::ranges::any_of(death_event_packets, [](const auto& packet) {
        const auto decoded = DecodeMessage(packet.second);
        return decoded && decoded->kind == Kind::Event && decoded->events == std::vector<u32>{12301800};
    }));
    sent_packets.clear();
    WriteValue<s32>(address(local), 0xA4, 1);
    WriteValue<s32>(address(local_module_data), 0xF8, 0);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));
    for (const auto& packet : death_event_packets)
        OnBbscPartyPacket({"host", 41}, packet.second.data(), packet.second.size());
    TickBbscPartyEventDelivery();
    auto earned_after_death = LoadJournal(BbscPartyHomePath(original_key), original_key, ec);
    assert(earned_after_death && earned_after_death->pending == (std::vector<u32>{12201800, 12301800}));

    // During loading, the selected identity reader can be unavailable. The cached exact key still
    // binds persistence to the same journal and does not close eligibility or write foreign state.
    WriteValue<s32>(address(local), 0xA4, 0);
    WriteValue<s32>(address(local_module_data), 0xF8, 1000);
    WriteValue<u8>(address(global), 0x08, 0);
    set_catalog_flag(12301700);
    OnBbscPartyRoomState({34, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"guest", 42}}}));
    bbsc_confirmed_party_peers.front().last_send_ms = SteadyNowMs() - 1'000;
    TickBbscPartyEventDelivery();
    auto loading_event_packets = sent_packets;
    sent_packets.clear();
    WriteValue<s32>(address(local), 0xA4, 1);
    WriteValue<u64>(address(local), 0x688, 0);
    WriteValue<u8>(address(global), 0x08, 1);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));
    for (const auto& packet : loading_event_packets)
        OnBbscPartyPacket({"host", 41}, packet.second.data(), packet.second.size());
    TickBbscPartyEventDelivery();
    auto earned_after_loading = LoadJournal(BbscPartyHomePath(original_key), original_key, ec);
    assert(bbsc_guest_party_admission && earned_after_loading &&
           earned_after_loading->pending == (std::vector<u32>{12201800, 12301800, 12301700}));
    WriteValue<u64>(address(local), 0x688, 4127);
    WriteValue<u8>(address(global), 0x08, 0);
    WriteValue<s32>(address(local_module_data), 0xF8, 1000);

    // The native guest session ends before Matching2 republishes the room; terminal Leave still
    // closes the exact channel and queued post-exit events are not persisted.
    WriteValue<s32>(address(matching), 0x124, 0);
    sent_packets.clear();
    TickBbscPartyEventDelivery();
    auto leave_packets = sent_packets;
    assert(!bbsc_guest_party_admission && std::ranges::any_of(leave_packets, [](const auto& packet) {
        const auto decoded = DecodeMessage(packet.second);
        return decoded && decoded->kind == Kind::Leave;
    }));
    sent_packets.clear();
    WriteValue<s32>(address(local), 0xA4, 0);
    OnBbscPartyRoomState({34, 77, true, "host", {"guest"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"guest", 42}}}));
    for (const auto& packet : leave_packets)
        if (const auto decoded = DecodeMessage(packet.second); decoded && decoded->kind == Kind::Leave)
            OnBbscPartyPacket({"guest", 42}, packet.second.data(), packet.second.size());
    TickBbscPartyEventDelivery();
    assert(std::ranges::any_of(bbsc_confirmed_party_peers, [](const auto& entry) { return entry.closed; }));
    sent_packets.clear();
    TickBbscPartyEventDelivery();
    assert(sent_packets.empty()); // No events are earned or delivered after confirmed room exit.
    WriteValue<s32>(address(local), 0xA4, 1);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}}); // Deliberately retain the stale room.
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));
    Core::Bbsc::Party::Message after_exit;
    after_exit.party = probe->challenge;
    after_exit.admission = ticket.nonce;
    after_exit.sequence = 4;
    after_exit.kind = Kind::Event;
    after_exit.events = {12301900};
    const auto after_exit_bytes = EncodeMessage(after_exit);
    OnBbscPartyPacket({"host", 41}, after_exit_bytes.data(), after_exit_bytes.size());
    TickBbscPartyEventDelivery();
    const auto after_exit_record = LoadJournal(BbscPartyHomePath(original_key), original_key, ec);
    assert(after_exit_record && std::ranges::find(after_exit_record->pending, 12301900) ==
                                    after_exit_record->pending.end());
    bbsc_confirmed_party_peers.clear();
    bbsc_guest_party_admission.reset();
    bbsc_admitted_join.reset();
    OnBbscPartyRoomState({34, 0, false, "", {}});
    WriteValue<s32>(address(local), 0xA4, 0);
    WriteValue<s32>(address(matching), 0x124, 0);
    std::filesystem::remove(BbscPartyHomePath(original_key), ec);
    assert(!ec);
    // Before a managed admission is known, missing room publication must not close the vanilla
    // summon curtain. A real managed room loss above still releases its own cover.
    bbsc_guest_party_admission.reset();
    bbsc_admitted_join.reset();
    bbsc_load_cover = {};
    bbsc_load_cover.active = true;
    bbsc_load_cover.why = "summon";
    TickBbscPartyEventDelivery();
    assert(bbsc_load_cover.active);
    bbsc_load_cover = {};
    admission_packets.clear();
    flag_manager = previous_delivery_flag_manager;
    WriteValue<s32>(address(matching), 0x124, 6);
    OnBbscPartyRoomState({34, 77, false, "host", {"host"}});
    assert(SetBbscPartyPeers(std::array<Peer, 1>{{{"host", 41}}}));

    const auto start_admission = [&](s32 context, u64 private_generation) {
        ticket.context = context;
        ticket.nonce += 1;
        ticket.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        bbsc_test_admission_tickets = {ticket};
        bbsc_test_admission_resolutions.clear();
        admission_packets.clear();
        bbsc_test_capability_proofs = {};
        bbsc_test_capability_proofs.observation.private_attempt = {
            Libraries::Http::BbscPrivateAttemptState::Ready, private_generation};
        bbsc_test_capability_proofs.observation.own_sign = {
            .valid = true, .session_id = "guest-session", .user_id = 4127,
            .character_id = 2723151752040083LL, .summon_type = 0};
        bbsc_test_capability_proofs.own.fill(0x11);
        TickBbscAdmissionCoordinator();
        assert(admission_packets.size() == 1);
        const auto sent_probe = DecodeCapability(admission_packets[0].second);
        assert(sent_probe && sent_probe->kind == CapabilityKind::Probe);
        return *sent_probe;
    };
    const auto reject_confirmation = [&](const Capability& sent_probe) {
        Capability bad_confirm;
        bad_confirm.kind = CapabilityKind::Confirm;
        bad_confirm.room = sent_probe.room;
        bad_confirm.challenge = sent_probe.challenge;
        bad_confirm.own_proof.fill(0x33);
        bad_confirm.selected_proof = bbsc_test_capability_proofs.own;
        const auto bytes = EncodeCapability(bad_confirm);
        OnBbscPartyPacket({"host", 41}, bytes.data(), bytes.size());
        TickBbscAdmissionCoordinator();
        assert(bbsc_test_admission_resolutions.size() == 1 &&
               !bbsc_test_admission_resolutions[0].second);
        assert(!Core::Bbsc::Party::LoadJournal(BbscPartyHomePath(original_key), original_key, ec));
    };

    auto sent_probe = start_admission(34, 13);
    bbsc_test_capability_proofs.observation.own_sign.summon_type = 1; // stale native role
    reject_confirmation(sent_probe);
    sent_probe = start_admission(34, 14);
    bbsc_test_capability_proofs.observation.private_attempt.generation += 1; // stale request generation
    reject_confirmation(sent_probe);
    sent_probe = start_admission(34, 15);
    bbsc_test_capability_proofs.observation.own_sign.character_id += 1; // sign belongs to another save
    reject_confirmation(sent_probe);
    sent_probe = start_admission(34, 16);
    mock_receipt_sha.assign(64, '0'); // selected-save hash changed after the challenge
    reject_confirmation(sent_probe);
    mock_receipt_sha = good_receipt_hash;
    sent_probe = start_admission(34, 17);
    mock_receipt_override_path = (folder / "userdata0002").string(); // selected path changed
    reject_confirmation(sent_probe);
    mock_receipt_override_path.clear();
    sent_probe = start_admission(34, 18);
    WriteValue<u64>(address(local), 0x690, 2723151752040084ULL); // selected character changed
    reject_confirmation(sent_probe);
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    sent_probe = start_admission(34, 19);
    Capability stale_confirm;
    stale_confirm.kind = CapabilityKind::Confirm;
    stale_confirm.room = sent_probe.room;
    stale_confirm.challenge = sent_probe.challenge;
    stale_confirm.own_proof.fill(0x33);
    stale_confirm.selected_proof = bbsc_test_capability_proofs.own;
    const auto stale_confirm_bytes = EncodeCapability(stale_confirm);
    Journal prior_complete;
    prior_complete.key = original_key;
    prior_complete.party.fill(0x5a);
    prior_complete.origin = {0x18010000, 10, 20, 30, 0.5f, 2412950};
    prior_complete.fallback = {0x18010000, 100, 200, 300, 0.75f, 2412950};
    {
        auto lease = CharacterLease::Acquire(BbscPartyHomePath(original_key), ec);
        assert(lease);
        assert(StoreJournal(*lease, prior_complete, ec));
        prior_complete.admission = 99;
        prior_complete.phase = Phase::Visiting;
        assert(StoreJournal(*lease, prior_complete, ec));
        prior_complete.phase = Phase::Returning;
        assert(StoreJournal(*lease, prior_complete, ec));
        prior_complete.phase = Phase::Complete;
        assert(StoreJournal(*lease, prior_complete, ec));
    }
    bbsc_test_admission_resolution_success = false;
    OnBbscPartyPacket({"host", 41}, stale_confirm_bytes.data(), stale_confirm_bytes.size());
    TickBbscAdmissionCoordinator();
    bbsc_test_admission_resolution_success = true;
    assert(bbsc_test_admission_resolutions.size() == 2 &&
           bbsc_test_admission_resolutions[0].second &&
           !bbsc_test_admission_resolutions[1].second);
    assert(Core::Bbsc::Party::LoadJournal(BbscPartyHomePath(original_key), original_key, ec) ==
           prior_complete);
    admission_peers.clear();
    bbsc_native_coop_hooks_ready = false;
    OnBbscPartyRoomState({34, 0, false, {}, {}});
    WriteValue<s32>(address(matching), 0x124, 0);
    bbsc_test_capability_proofs = {};
    if (previous_bbsc_setting) setenv("BB_BBSC", previous_bbsc_setting->c_str(), 1);
    else unsetenv("BB_BBSC");
    TestCapabilityAdmissionHostTick();

    auto home = PrepareBbscNativePartyHome({4, 5}, ec, true);
    assert(home && !ec);
    assert(home->record.key == original_key && home->record.admission == 0);
    assert(home->record.phase == Core::Bbsc::Party::Phase::Prepared);
    assert(home->record.origin.map == 0x18010000 && home->record.origin.x == 10);
    assert(home->record.fallback.map == 0x18010000 && home->record.fallback.x == 100);
    assert(home->record.fallback.lamp == 2412950);
    assert(std::filesystem::is_regular_file(selected_backup));
    native_respawn_map = 0x18020000; // Native API resolves a point whose map is not currently loaded.
    assert(ResolveBbscRespawnPointMap(2412950) == native_respawn_map);
    native_respawn_map = 0x18010000;
    const auto journal_path = BbscPartyHomePath(original_key);
    assert(home->record.world_snapshot.bytes != 0);
    const auto bound_snapshot = Core::Bbsc::Party::LoadWorldSnapshot(
        journal_path, original_key, home->record.party, home->record.world_snapshot, ec);
    assert(bound_snapshot && *bound_snapshot == *SerializeBbscOwnWorldSnapshot());
    assert(Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec) == home->record);
    assert(!PrepareBbscNativePartyHome({6, 7}, ec) && ec); // Same-character lease is held.
    home.reset();
    WriteValue<float>(address(transform), 0x1E0, 999);
    assert(!PrepareBbscNativePartyHome({6, 7}, ec) && ec == std::errc::operation_in_progress);
    auto saved_home = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(saved_home && saved_home->origin.x == 10); // Never overwrite unresolved recovery.
    RestoreBbscOwnWorldAtLoad(true); // Prepared is not proof that the character visited a host.
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    assert(bbsc_own_world.size() == 1 && bbsc_own_world.at(12410) == saved_group &&
           bbsc_own_world_key == original_key);
    const char join_name[] = "JoinSession_White", other_name[] = "OnLeaveMagic";
    Core::GuestRegisterSnapshot dispatch{};
    dispatch.rsi = reinterpret_cast<u64>(other_name);
    assert(!ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi);
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi == 0);
    assert(Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec) == saved_home);
    auto* dispatch_code = static_cast<u8*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    assert(dispatch_code != MAP_FAILED);
    // Replay the native null-name rejection predicate through the actual hook provider.
    const std::array<u8, 23> dispatcher{
        0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x48, 0x85, 0xF6, 0x74, 0x06,
        0xC7, 0x07, 0x01, 0, 0, 0, 0x31, 0xC0, 0x41, 0x5F, 0x5D, 0xC3};
    std::memcpy(dispatch_code, dispatcher.data(), dispatcher.size());
    const auto guard = +[](u64, const Core::GuestRegisterSnapshot* regs) {
        ApplyBbscPartyRecoveryJoinGuard(*regs);
    };
    assert(Core::InstallGuestCodeHook(dispatch_code, std::span{dispatcher}.first(6), 0, guard));
    using Dispatch = s32 PS4_SYSV_ABI (*)(s32*, const char*);
    s32 action = 0;
    assert(reinterpret_cast<Dispatch>(dispatch_code)(&action, join_name) == 0 && action == 0);

    // Only a pending, matching native home record may restore its durable own-world payload.
    const auto snapshot_path = Core::Bbsc::Party::WorldSnapshotPath(
        journal_path, Core::Bbsc::Party::PartyId{4, 5});
    std::ifstream snapshot_input(snapshot_path, std::ios::binary);
    const std::string snapshot_intact((std::istreambuf_iterator<char>(snapshot_input)), {});
    assert(!snapshot_intact.empty());
    saved_home->phase = Core::Bbsc::Party::Phase::Visiting;
    auto phase_lease = Core::Bbsc::Party::CharacterLease::Acquire(journal_path, ec);
    assert(phase_lease && Core::Bbsc::Party::StoreJournal(*phase_lease, *saved_home, ec));
    assert(Core::Bbsc::Party::BeginReturnSaveFence(*saved_home, mock_receipt_generation));
    saved_home->native_return_selected = true;
    assert(Core::Bbsc::Party::StoreJournal(*phase_lease, *saved_home, ec));
    phase_lease.reset();
    auto managed_saved = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(managed_saved && managed_saved->phase == Core::Bbsc::Party::Phase::Returning &&
           managed_saved->native_return_selected);
    WriteValue<u8>(address(global), 0x1592, 0); // still in the host world
    RestoreBbscOwnWorldAtLoad(true);
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    assert(Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec) == managed_saved);
    WriteValue<u8>(address(global), 0x1592, 1);
    RestoreBbscOwnWorldAtLoad(false); // earlier generic boundary must defer managed recovery
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    WriteValue<u64>(image_base, WorldFlagManPointerOffset, 0);
    RestoreBbscOwnWorldAtLoad(true); // a missing/not-ready flag manager is not a restore target
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    WriteValue<u64>(image_base, WorldFlagManPointerOffset, address(flag_manager));
    WriteValue<u32>(address(flag_manager), 0x20, BbscWorldGroupBytes - 1);
    RestoreBbscOwnWorldAtLoad(true); // A non-null but malformed manager must preserve the snapshot.
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    assert(bbsc_own_world.size() == 1 && bbsc_own_world.at(12410) == saved_group &&
           bbsc_own_world_key == original_key);
    WriteValue<u32>(address(flag_manager), 0x20, BbscWorldGroupBytes);

    // Exercise the actual recovery tick against a synthetic native image. Each direct native
    // call is replaced by a counted ABI stub; identity/journal guards run through production code.
    const auto native_stub = [&](u64 offset, u64 target) {
        const u64 site = image_base + offset;
        const u64 native_page = site & ~u64{4095};
        assert(!mprotect(reinterpret_cast<void*>(native_page), 4096,
                          PROT_READ | PROT_WRITE | PROT_EXEC));
        const std::array<u8, 12> jump{0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
        std::memcpy(reinterpret_cast<void*>(site), jump.data(), jump.size());
        WriteValue<u64>(site, 2, target);
    };
    native_stub(SelectSummonedPlacementOffset,
                reinterpret_cast<u64>(TestRecoveryPlacementSelector));
    native_stub(StageTransitionOffset,
                reinterpret_cast<u64>(TestRecoveryStageTransition));
    auto recovery_record = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(recovery_record && recovery_record->phase == Core::Bbsc::Party::Phase::Returning &&
           recovery_record->native_return_selected && recovery_record->world_snapshot.bytes);
    WriteValue<u8>(address(global), 8, 0);
    WriteValue<u8>(address(global), 0x1592, 1);
    WriteValue<u32>(address(global), 0x0C, recovery_record->fallback.map);
    bbsc_offline_return_key = original_key;
    bbsc_offline_return_party = recovery_record->party;
    bbsc_offline_return_selected = true;
    bbsc_offline_return_warp_attempts = 0;
    bbsc_offline_return_last_attempt_ms = 0;
    bbsc_managed_return_save_tracker.Arm(original_key, recovery_record->save_generation,
                                          recovery_record->fallback, true);
    const int stage_before = recovery_stage_calls;
    TickBbscOfflineManagedReturnWarp();
    assert(recovery_selector_calls == 1 && recovery_stage_calls == stage_before + 1);
    assert(bbsc_offline_return_warp_attempts == 1);

    // A pending load, foreign selected character, missing journal and corrupt journal each
    // withhold the actual selector/StageTransition calls and leave durable recovery pending.
    WriteValue<u8>(address(global), 8, 1);
    bbsc_managed_return_save_tracker.Arm(original_key, recovery_record->save_generation,
                                          recovery_record->fallback, true);
    TickBbscOfflineManagedReturnWarp();
    assert(recovery_selector_calls == 1 && recovery_stage_calls == stage_before + 1 &&
           bbsc_managed_return_save_tracker.Armed());
    WriteValue<u8>(address(global), 8, 0);
    WriteValue<u64>(address(local), 0x690, 2723151752040084ULL);
    bbsc_managed_return_save_tracker.Arm(original_key, recovery_record->save_generation,
                                          recovery_record->fallback, true);
    TickBbscOfflineManagedReturnWarp();
    assert(recovery_selector_calls == 1 && recovery_stage_calls == stage_before + 1);
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    auto held_journal_path = journal_path.string() + ".fixture-held";
    fs::rename(journal_path, held_journal_path);
    bbsc_managed_return_save_tracker.Arm(original_key, recovery_record->save_generation,
                                          recovery_record->fallback, true);
    TickBbscOfflineManagedReturnWarp();
    assert(recovery_selector_calls == 1 && recovery_stage_calls == stage_before + 1);
    fs::rename(held_journal_path, journal_path);
    std::ifstream recovery_journal_input(journal_path, std::ios::binary);
    const std::string recovery_journal_bytes((std::istreambuf_iterator<char>(recovery_journal_input)), {});
    recovery_journal_input.close();
    {
        std::ofstream out(journal_path, std::ios::binary | std::ios::trunc);
        out << 'X';
    }
    bbsc_managed_return_save_tracker.Arm(original_key, recovery_record->save_generation,
                                          recovery_record->fallback, true);
    TickBbscOfflineManagedReturnWarp();
    assert(recovery_selector_calls == 1 && recovery_stage_calls == stage_before + 1);
    {
        std::ofstream out(journal_path, std::ios::binary | std::ios::trunc);
        out.write(recovery_journal_bytes.data(), recovery_journal_bytes.size());
    }
    recovery_record = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(recovery_record && recovery_record->phase == Core::Bbsc::Party::Phase::Returning);

    // A native selector rejection consumes a bounded attempt but cannot complete or erase the
    // journal; the next attempt can retry only after the delay.
    bbsc_offline_return_key = original_key;
    bbsc_offline_return_party = recovery_record->party;
    bbsc_offline_return_selected = true;
    bbsc_offline_return_warp_attempts = 0;
    bbsc_offline_return_last_attempt_ms = SteadyNowMs() - 6'000;
    bbsc_managed_return_save_tracker.Arm(original_key, recovery_record->save_generation,
                                          recovery_record->fallback, true);
    recovery_selector_succeeds = false;
    const int failed_selector_stage_count = recovery_stage_calls;
    const int failed_selector_count = recovery_selector_calls;
    TickBbscOfflineManagedReturnWarp();
    assert(recovery_selector_calls == failed_selector_count + 1 &&
           recovery_stage_calls == failed_selector_stage_count && bbsc_offline_return_warp_attempts == 1);
    recovery_selector_succeeds = true;
    recovery_record = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(recovery_record && recovery_record->phase == Core::Bbsc::Party::Phase::Returning &&
           recovery_record->world_snapshot.bytes);

    // Re-entering the same character/party load must not reset the three-attempt ceiling.
    bbsc_offline_return_key = original_key;
    bbsc_offline_return_party = recovery_record->party;
    bbsc_offline_return_selected = true;
    bbsc_offline_return_warp_attempts = 2;
    bbsc_offline_return_last_attempt_ms = SteadyNowMs() - 6'000;
    RestoreBbscOwnWorldAtLoad(true);
    assert(bbsc_offline_return_warp_attempts == 2);
    bbsc_offline_return_last_attempt_ms = SteadyNowMs() - 6'000;
    const int calls_before_cap = recovery_stage_calls;
    TickBbscOfflineManagedReturnWarp();
    assert(bbsc_offline_return_warp_attempts == 3 && recovery_stage_calls == calls_before_cap + 1);
    bbsc_offline_return_last_attempt_ms = SteadyNowMs() - 6'000;
    TickBbscOfflineManagedReturnWarp();
    assert(bbsc_offline_return_warp_attempts == 3 && recovery_stage_calls == calls_before_cap + 1);
    recovery_record = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(recovery_record && recovery_record->phase == Core::Bbsc::Party::Phase::Returning &&
           recovery_record->world_snapshot.bytes);
    std::fill(live_flags.begin(), live_flags.end(), 0xFF);
    bbsc_own_world.clear();
    bbsc_own_world.emplace(12410, saved_group);
    bbsc_own_world_key = original_key;

    // Neither corrupt payload bytes nor a foreign selected character may mutate the live flags.
    {
        std::ofstream out(snapshot_path, std::ios::binary | std::ios::trunc);
        out.write(snapshot_intact.data(), static_cast<std::streamsize>(snapshot_intact.size() - 1));
    }
    RestoreBbscOwnWorldAtLoad(true);
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    assert(bbsc_own_world.size() == 1 && bbsc_own_world.at(12410) == saved_group &&
           bbsc_own_world_key == original_key);
    {
        std::ifstream in(snapshot_path, std::ios::binary);
        const std::string truncated((std::istreambuf_iterator<char>(in)), {});
        assert(truncated.size() + 1 == snapshot_intact.size());
        std::ofstream out(snapshot_path, std::ios::binary | std::ios::trunc);
        out.write(snapshot_intact.data(), static_cast<std::streamsize>(snapshot_intact.size()));
    }
    WriteValue<u64>(address(local), 0x690, 2723151752040084ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040084ULL);
    save();
    RestoreBbscOwnWorldAtLoad(true);
    assert(std::all_of(live_flags.begin(), live_flags.end(), [](u8 value) { return value == 0xFF; }));
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040083ULL);
    save();

    RestoreBbscOwnWorldAtLoad(true);
    assert(live_flags == saved_group);
    assert(bbsc_own_world.empty() && !bbsc_own_world_key && !bbsc_own_world_slot);
    auto retained = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(retained && retained->phase == Core::Bbsc::Party::Phase::Returning &&
           retained->save_generation == 12);
    // A synthetic live transform that does not match the resolved native point cannot confirm
    // destination, so neither serialization nor a newer receipt may complete this return.
    TickBbscManagedReturnSaveReceipt();
    assert(!bbsc_managed_return_save_tracker.DestinationConfirmed());
    std::array<u8, 0x40> save_end_stack{};
    Core::GuestRegisterSnapshot save_begin{};
    save_begin.rdi = address(global);
    save_begin.rsi = address(bytes);
    save_begin.rdx = 0x140000;
    ObserveBbscManagedReturnSerialization(NativeSaveSerializeOffset, save_begin);
    WriteValue<u64>(address(save_end_stack), 0x28, address(bytes));
    Core::GuestRegisterSnapshot save_end{};
    save_end.rsp = address(save_end_stack);
    save_end.rbx = 1;
    ObserveBbscManagedReturnSerialization(NativeSaveSerializeEndOffset, save_end);
    const auto serialized = ReadBbscSerializedSnapshot(bytes);
    assert(serialized);
    assert(!bbsc_managed_return_save_tracker.Saved());
    retained = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(retained && retained->phase == Core::Bbsc::Party::Phase::Returning &&
           retained->save_generation == 12);
    mock_receipt_generation = 13;
    mock_receipt_sha = serialized->sha256;
    TickBbscManagedReturnSaveReceipt();
    assert(!bbsc_managed_return_save_tracker.Saved());
    retained = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(retained && retained->phase == Core::Bbsc::Party::Phase::Returning &&
           retained->save_generation == 12);
    const auto retained_payload = Core::Bbsc::Party::LoadWorldSnapshot(
        journal_path, original_key, retained->party, retained->world_snapshot, ec);
    assert(retained_payload && bound_snapshot && *retained_payload == *bound_snapshot);
    {
        std::ifstream in(snapshot_path, std::ios::binary);
        assert(std::string((std::istreambuf_iterator<char>(in)), {}) == snapshot_intact);
    }
    WriteValue<u8>(address(global), 0x1592, 1);
    auto held_home = Core::Bbsc::Party::CharacterLease::Acquire(journal_path, ec);
    assert(held_home);
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi == 0); // Held in another owner.
    held_home.reset();
    TickBbscManagedReturnSaveReceipt();
    auto completed_home = Core::Bbsc::Party::CharacterLease::Acquire(journal_path, ec);
    assert(completed_home);
    auto completed_record = Core::Bbsc::Party::LoadJournal(journal_path, original_key, ec);
    assert(completed_record && completed_record->phase == Core::Bbsc::Party::Phase::Returning &&
           completed_record->save_generation == 12);
    completed_record->phase = Core::Bbsc::Party::Phase::Complete;
    assert(Core::Bbsc::Party::StoreJournal(*completed_home, *completed_record, ec));
    completed_home.reset();
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(!ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi);
    assert(reinterpret_cast<Dispatch>(dispatch_code)(&action, join_name) == 0 && action == 1);
    assert(!munmap(dispatch_code, 4096));

    // Exercise the production cycle observers through fresh 6->7 and saturated
    // 7->7 transitions, including the window between SerializeEnd and copy receipt.
    constexpr u64 cycle_subject = 2723151752040086ULL;
    WriteValue<u64>(address(local), 0x690, cycle_subject);
    WriteValue<u64>(address(bytes), 0x24C, cycle_subject);
    WriteValue<s32>(address(slots), 0x68, 6);
    WriteValue<s32>(address(bytes), 0x254, 6);
    save();
    mock_receipt_generation = 12;
    mock_receipt_sha = ReadBbscSerializedSnapshot(bytes)->sha256;
    auto native6 = ReadBbscNativeCharacterIdentity();
    assert(native6 && native6->capped_cycle == 6 && native6->persisted_capped_cycle == 6);
    auto native6_key = BbscCycleLookupKey(*native6);
    auto native6_lease = Core::Bbsc::Party::CharacterLease::Acquire(
        BbscPartyHomePath(native6_key), ec);
    assert(native6_lease);
    Core::Bbsc::Party::ManagedCycleEpoch native6_stable;
    native6_stable.key = native6_key;
    native6_stable.key.cycle = 6;
    native6_stable.native_cycle = 7;
    assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*native6_lease, native6_stable, ec));
    native6_lease.reset();
    Core::GuestRegisterSnapshot cycle_registers{};
    ObserveBbscManagedCycleEvidence(NativeCycleLuaIntentOffset, cycle_registers);
    auto cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Intent);
    WriteValue<s32>(address(slots), 0x68, 7);
    cycle_registers.rcx = address(slots) + 0x68;
    cycle_registers.rdi = 7;
    cycle_registers.rsi = 6;
    cycle_registers.rdx = 7;
    ObserveBbscManagedCycleEvidence(NativeCycleAppliedOffset, cycle_registers);
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Applied);
    WriteValue<s32>(address(bytes), 0x254, 7);
    Core::GuestRegisterSnapshot cycle_begin{};
    cycle_begin.rdi = address(global);
    cycle_begin.rsi = address(bytes);
    cycle_begin.rdx = 0x140000;
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeOffset, cycle_begin);
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Applied);
    std::array<u8, 0x40> cycle_end_stack{};
    WriteValue<u64>(address(cycle_end_stack), 0x28, address(bytes));
    Core::GuestRegisterSnapshot cycle_end{};
    cycle_end.rsp = address(cycle_end_stack);
    cycle_end.rbx = 1;
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeEndOffset, cycle_end);
    auto cycle_serialized = ReadBbscSerializedSnapshot(bytes);
    assert(cycle_serialized);
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage ==
           Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized);
    TickBbscManagedCycleReceipt(); // Must retain in-process provenance until copy receipt.
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage ==
           Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized);
    {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    mock_receipt_generation = 13;
    mock_receipt_sha = cycle_serialized->sha256;
    TickBbscManagedCycleReceipt();
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Stable &&
           cycle_state->key.cycle == 0x80000001u && cycle_state->epoch == 1);

    // A 7->7 serialize may have identical bytes. Same-process producer provenance
    // plus a newer receipt commits it; hash-only restart evidence remains ambiguous.
    native6 = ReadBbscNativeCharacterIdentity();
    assert(native6 && native6->capped_cycle == 7 && native6->persisted_capped_cycle == 7);
    ObserveBbscManagedCycleEvidence(NativeCycleLuaIntentOffset, cycle_registers);
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Intent &&
           cycle_state->target_key_cycle == 0x80000002u);
    cycle_registers.rcx = address(slots) + 0x68;
    cycle_registers.rdi = 8;
    cycle_registers.rsi = 7;
    cycle_registers.rdx = 7;
    ObserveBbscManagedCycleEvidence(NativeCycleAppliedOffset, cycle_registers);
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeOffset, cycle_begin);
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeEndOffset, cycle_end);
    cycle_serialized = ReadBbscSerializedSnapshot(bytes);
    assert(cycle_serialized);
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage ==
           Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized &&
           cycle_state->serialized_sha256 == cycle_state->baseline_sha256);
    TickBbscManagedCycleReceipt();
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage ==
           Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized);
    mock_receipt_generation = 14;
    mock_receipt_sha = cycle_serialized->sha256;
    TickBbscManagedCycleReceipt();
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Stable &&
           cycle_state->key.cycle == 0x80000002u && cycle_state->epoch == 2);

    // The same epoch's ordinary save also has a preimage-to-copy window; retain
    // its save evidence until the newer receipt refreshes the selected hash.
    native6 = ReadBbscNativeCharacterIdentity();
    assert(native6 && native6->capped_cycle == 7 && native6->persisted_capped_cycle == 7);
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeOffset, cycle_begin);
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeEndOffset, cycle_end);
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::SaveSerialized);
    TickBbscManagedCycleReceipt();
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::SaveSerialized);
    mock_receipt_generation = 15;
    mock_receipt_sha = cycle_serialized->sha256;
    TickBbscManagedCycleReceipt();
    cycle_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(native6_key), native6_key, ec);
    assert(cycle_state && cycle_state->stage == Core::Bbsc::Party::ManagedCycleStage::Stable &&
           cycle_state->key.cycle == 0x80000002u && cycle_state->epoch == 2 &&
           cycle_state->serialized_generation == 15);
    std::puts("PASS: production cycle observers preserve fresh 6->7 and 7->7 transitions through copy receipts");

    // A native-6 selected preimage after restart safely cancels an uncommitted
    // Intent/Applied/serialized candidate; foreign or unknown hashes remain blocked.
    for (const auto stage : {Core::Bbsc::Party::ManagedCycleStage::Intent,
                             Core::Bbsc::Party::ManagedCycleStage::Applied,
                             Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized}) {
        const u64 subject = cycle_subject + static_cast<u64>(stage) + 1;
        WriteValue<u64>(address(local), 0x690, subject);
        WriteValue<u64>(address(bytes), 0x24C, subject);
        WriteValue<s32>(address(slots), 0x68, 6);
        WriteValue<s32>(address(bytes), 0x254, 6);
        save();
        const auto saved_identity = ReadBbscNativeCharacterIdentity();
        assert(saved_identity && saved_identity->persisted_capped_cycle == 6);
        auto resume_key = BbscCycleLookupKey(*saved_identity);
        auto resume_lease = Core::Bbsc::Party::CharacterLease::Acquire(
            BbscPartyHomePath(resume_key), ec);
        assert(resume_lease);
        Core::Bbsc::Party::ManagedCycleEpoch pending;
        pending.key = resume_key;
        pending.key.cycle = 6;
        pending.native_cycle = 7;
        pending.stage = Core::Bbsc::Party::ManagedCycleStage::Stable;
        assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*resume_lease, pending, ec));
        auto intent = Core::Bbsc::Party::BeginManagedCycleIntent(
            pending, 6, mock_receipt_generation, saved_identity->snapshot_sha256);
        assert(intent);
        assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*resume_lease, *intent, ec));
        if (stage != Core::Bbsc::Party::ManagedCycleStage::Intent) {
            intent = Core::Bbsc::Party::ObserveManagedCycleApplied(*intent, 6, 7);
            assert(intent);
            assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*resume_lease, *intent, ec));
        }
        if (stage == Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized) {
            const std::string distinct_hash(64, 'a');
            intent = Core::Bbsc::Party::ObserveManagedCycleSerializeEnd(*intent,
                distinct_hash, mock_receipt_generation);
            assert(intent && intent->stage == stage);
            assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*resume_lease, *intent, ec));
        }
        resume_lease.reset();
        mock_receipt_sha = saved_identity->snapshot_sha256;
        TickBbscManagedCycleReceipt();
        const auto resumed = Core::Bbsc::Party::LoadManagedCycleEpoch(
            BbscPartyHomePath(resume_key), resume_key, ec);
        assert(resumed && resumed->stage == Core::Bbsc::Party::ManagedCycleStage::Stable &&
               resumed->key.cycle == 6);
    }

    // A new process can recognize a distinct serialized postimage but must issue a
    // fresh SerializeEnd/receipt before advancing. Its fresh-process generation is
    // lower than the persisted generation and must not strand the candidate.
    constexpr u64 resumed_subject = 2723151752040091ULL;
    WriteValue<u64>(address(local), 0x690, resumed_subject);
    WriteValue<u64>(address(bytes), 0x24C, resumed_subject);
    WriteValue<s32>(address(slots), 0x68, 7);
    WriteValue<s32>(address(bytes), 0x254, 7);
    save();
    auto resumed_identity = ReadBbscNativeCharacterIdentity();
    assert(resumed_identity && resumed_identity->capped_cycle == 7 &&
           resumed_identity->persisted_capped_cycle == 7);
    auto resumed_key = BbscCycleLookupKey(*resumed_identity);
    auto resumed_lease = Core::Bbsc::Party::CharacterLease::Acquire(
        BbscPartyHomePath(resumed_key), ec);
    assert(resumed_lease);
    Core::Bbsc::Party::ManagedCycleEpoch resumed_stable;
    resumed_stable.key = resumed_key;
    resumed_stable.key.cycle = 0x80000001u;
    resumed_stable.epoch = 1;
    resumed_stable.target_key_cycle = resumed_stable.key.cycle;
    resumed_stable.serialized_generation = 99;
    resumed_stable.serialized_sha256 = std::string(64, 'a');
    assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*resumed_lease, resumed_stable, ec));
    auto resumed_intent = Core::Bbsc::Party::BeginManagedCycleIntent(
        resumed_stable, 7, 99, resumed_stable.serialized_sha256);
    assert(resumed_intent && Core::Bbsc::Party::StoreManagedCycleEpoch(
        *resumed_lease, *resumed_intent, ec));
    auto resumed_applied = Core::Bbsc::Party::ObserveManagedCycleApplied(*resumed_intent, 7, 7);
    assert(resumed_applied && Core::Bbsc::Party::StoreManagedCycleEpoch(
        *resumed_lease, *resumed_applied, ec));
    auto resumed_serialized = Core::Bbsc::Party::ObserveManagedCycleSerializeEnd(
        *resumed_applied, resumed_identity->snapshot_sha256, 99);
    assert(resumed_serialized && resumed_serialized->stage ==
           Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized);
    assert(Core::Bbsc::Party::StoreManagedCycleEpoch(*resumed_lease, *resumed_serialized, ec));
    resumed_lease.reset();
    mock_receipt_generation = 1; // A new runtime's ABI3 counter restarts at one.
    mock_receipt_sha = resumed_identity->snapshot_sha256;
    assert(!bbsc_cycle_process_provenance.active);
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeOffset, cycle_begin);
    auto resumed_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(resumed_key), resumed_key, ec);
    assert(resumed_state && resumed_state->stage == Core::Bbsc::Party::ManagedCycleStage::Applied &&
           resumed_state->baseline_sha256 == resumed_identity->snapshot_sha256 &&
           bbsc_cycle_process_provenance.active);
    auto failed_cycle_end = cycle_end;
    failed_cycle_end.rbx = 0;
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeEndOffset, failed_cycle_end); // No native completion observation.
    resumed_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(resumed_key), resumed_key, ec);
    assert(resumed_state && resumed_state->stage == Core::Bbsc::Party::ManagedCycleStage::Applied);
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeOffset, cycle_begin); // Retry the recognized postimage.
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeEndOffset, cycle_end);
    cycle_serialized = ReadBbscSerializedSnapshot(bytes);
    assert(cycle_serialized);
    resumed_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(resumed_key), resumed_key, ec);
    assert(resumed_state && resumed_state->stage ==
           Core::Bbsc::Party::ManagedCycleStage::TransitionSerialized);
    {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    mock_receipt_generation = 2;
    mock_receipt_sha = cycle_serialized->sha256;
    TickBbscManagedCycleReceipt();
    resumed_state = Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(resumed_key), resumed_key, ec);
    assert(resumed_state && resumed_state->stage == Core::Bbsc::Party::ManagedCycleStage::Stable &&
           resumed_state->key.cycle == 0x80000002u && resumed_state->epoch == 2);
    std::puts("PASS: recognized postimage resumes with fresh serialization and process-local receipt generation");

    mock_receipt_generation = 12;
    mock_receipt_sha = "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390";
    WriteValue<s32>(address(slots), 0x68, 0);
    WriteValue<s32>(address(bytes), 0x254, 0);
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040083ULL);
    save();
    mock_receipt_generation = 12;
    mock_receipt_sha = "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390";
    {
        std::fstream out(file, std::ios::binary | std::ios::in | std::ios::out);
        out.put('X');
    }
    assert(!ReadBbscNativeCharacterIdentity());
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi == 0);
    save();
    std::ifstream journal_input(journal_path, std::ios::binary);
    const std::string intact((std::istreambuf_iterator<char>(journal_input)), {});
    journal_input.close();
    auto corrupted = intact;
    corrupted[0] = 'X';
    {
        std::ofstream out(journal_path, std::ios::binary | std::ios::trunc);
        out.write(corrupted.data(), corrupted.size());
    }
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi == 0);
    {
        std::ifstream in(journal_path, std::ios::binary);
        assert(std::string((std::istreambuf_iterator<char>(in)), {}) == corrupted);
    }
    {
        std::ofstream out(journal_path, std::ios::binary | std::ios::trunc);
        out.write(intact.data(), intact.size());
    }
    const auto archived = journal_path.string() + ".old";
    fs::rename(journal_path, archived);
    fs::create_symlink(archived, journal_path);
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi == 0);
    assert(fs::file_size(archived) == intact.size());
    fs::remove(journal_path);
    fs::rename(archived, journal_path);
    WriteValue<u64>(address(local), 0x690, 2723151752040084ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040084ULL);
    save();
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(!ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi); // Another character's record.
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040083ULL);
    save();
    native_lamp_available = false;
    assert(!PrepareBbscNativePartyHome({8}, ec) && ec);
    native_lamp_available = true;
    assert(!mprotect(reinterpret_cast<void*>(page), 4096, PROT_READ | PROT_WRITE));
    assert(!mprotect(reinterpret_cast<void*>(map_page), 4096, PROT_READ | PROT_WRITE));
    assert(fs::file_size(file) == 0x140000);
    std::puts("PASS: native home capture durably binds selected character, origin and native fallback without overwriting recovery");
    WriteValue<s32>(address(slots), 0x68, 7);
    assert(ReadBbscNativeCharacterIdentity()->capped_cycle == 7); // evidence, not an uncapped key
    assert(ReadBbscNativeCharacterIdentity()->persisted_capped_cycle == 0);
    dispatch.rsi = reinterpret_cast<u64>(join_name);
    assert(ApplyBbscPartyRecoveryJoinGuard(dispatch) && dispatch.rsi == 0);
    WriteValue<s32>(address(bytes), 0x254, 7);
    WriteValue<u32>(address(bytes), 0x258, 0xDEADBEEF); // adjacent serialized field is not a cycle
    save();
    assert(ReadBbscNativeCharacterIdentity()->persisted_capped_cycle == 7);
    assert(!ReadBbscPartyCharacterKey()); // Capped 7 is not an uncapped cycle discriminator.
    for (s32 persisted_cycle : {-1, 8}) {
        WriteValue<s32>(address(bytes), 0x254, persisted_cycle);
        save();
        assert(!ReadBbscNativeCharacterIdentity());
    }
    WriteValue<s32>(address(bytes), 0x254, 0);
    save();
    // Exercise both production SerializeEnd observers in their installed order for a
    // tagged capped-7 epoch. The ordinary native key remains capped at 7; the durable
    // epoch sidecar supplies the managed key consumed by the return journal.
    constexpr u64 cap7_character_id = 2723151752040085ULL;
    WriteValue<u64>(address(local), 0x690, cap7_character_id);
    WriteValue<u64>(address(bytes), 0x24C, cap7_character_id);
    WriteValue<s32>(address(slots), 0x68, 7);
    WriteValue<s32>(address(bytes), 0x254, 7);
    save();
    auto cycle7_identity = ReadBbscNativeCharacterIdentity();
    assert(cycle7_identity && cycle7_identity->capped_cycle == 7 &&
           cycle7_identity->persisted_capped_cycle == 7);
    auto cycle_lookup = BbscPartyCharacterKey(*cycle7_identity);
    cycle_lookup.cycle = 6;
    auto managed_key = BbscPartyCharacterKey(*cycle7_identity);
    managed_key.cycle = 0x80000001u;
    Core::Bbsc::Party::ManagedCycleEpoch cycle7_state;
    cycle7_state.key = managed_key;
    cycle7_state.epoch = 1;
    cycle7_state.target_key_cycle = managed_key.cycle;
    cycle7_state.native_cycle = 7;
    cycle7_state.serialized_generation = mock_receipt_generation;
    cycle7_state.serialized_sha256 = cycle7_identity->snapshot_sha256;
    auto cycle_lease = Core::Bbsc::Party::CharacterLease::Acquire(
        BbscPartyHomePath(cycle_lookup), ec);
    assert(cycle_lease && Core::Bbsc::Party::StoreManagedCycleEpoch(*cycle_lease, cycle7_state, ec));
    cycle_lease.reset();
    assert(ReadBbscPartyCharacterKey() == managed_key);
    auto managed_home_path = BbscPartyHomePath(managed_key);
    Core::Bbsc::Party::Journal managed_return;
    managed_return.key = managed_key;
    managed_return.party = {4, 5, 6};
    managed_return.origin = {0x18010000, 10, 20, 30, 0.5f, -1};
    managed_return.fallback = {0x18010000, 100, 200, 300, 0.75f, 2412950};
    std::error_code cycle_home_ec;
    auto cycle_home_lease = Core::Bbsc::Party::CharacterLease::Acquire(managed_home_path,
                                                                        cycle_home_ec);
    assert(cycle_home_lease && Core::Bbsc::Party::StoreJournal(
        *cycle_home_lease, managed_return, cycle_home_ec));
    managed_return.phase = Core::Bbsc::Party::Phase::Visiting;
    assert(Core::Bbsc::Party::StoreJournal(*cycle_home_lease, managed_return, cycle_home_ec));
    managed_return.phase = Core::Bbsc::Party::Phase::Returning;
    managed_return.native_return_selected = true;
    assert(Core::Bbsc::Party::BeginReturnSaveFence(managed_return, mock_receipt_generation));
    assert(Core::Bbsc::Party::StoreJournal(*cycle_home_lease, managed_return, cycle_home_ec));
    cycle_home_lease.reset();
    mock_receipt_sha = cycle7_identity->snapshot_sha256;
    bbsc_own_world.clear();
    bbsc_own_world.emplace(12410, saved_group);
    bbsc_own_world_key = managed_key;
    bbsc_managed_return_save_tracker.Arm(managed_key, mock_receipt_generation,
        managed_return.fallback, true);
    assert(!mprotect(reinterpret_cast<void*>(page), 4096, PROT_READ | PROT_WRITE | PROT_EXEC));
    std::memcpy(reinterpret_cast<void*>(resolver), jump.data(), jump.size());
    WriteValue<u64>(resolver, 2, reinterpret_cast<u64>(TestNativeHomePoint));
    WriteValue<float>(address(transform), 0x1E0, 100);
    WriteValue<float>(address(transform), 0x1E4, 200);
    WriteValue<float>(address(transform), 0x1E8, 300);
    WriteValue<u8>(address(global), 0x1592, 1);
    TickBbscManagedReturnSaveReceipt(); // confirm the resolved native destination before this save begins
    assert(bbsc_managed_return_save_tracker.DestinationConfirmed());
    Core::GuestRegisterSnapshot managed_begin{};
    managed_begin.rdi = address(global);
    managed_begin.rsi = address(bytes);
    managed_begin.rdx = 0x140000;
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeOffset, managed_begin);
    assert(BbscManagedCycleAllowsReturnSerialization(*ReadBbscNativeCharacterIdentity(), managed_key));
    ObserveBbscManagedReturnSerialization(NativeSaveSerializeOffset, managed_begin);
    assert(bbsc_managed_return_save_tracker.Active());
    std::array<u8, 0x40> managed_end_stack{};
    WriteValue<u64>(address(managed_end_stack), 0x28, address(bytes));
    Core::GuestRegisterSnapshot managed_end{};
    managed_end.rsp = address(managed_end_stack);
    managed_end.rbx = 1;
    ObserveBbscManagedCycleEvidence(NativeSaveSerializeEndOffset, managed_end);
    ObserveBbscManagedReturnSerialization(NativeSaveSerializeEndOffset, managed_end);
    const auto cycle7_serialized = ReadBbscSerializedSnapshot(bytes);
    assert(cycle7_serialized && !bbsc_managed_return_save_tracker.Active());
    {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    mock_receipt_generation++;
    mock_receipt_sha = cycle7_serialized->sha256;
    TickBbscManagedReturnSaveReceipt(); // must wait while cycle observer has SaveSerialized
    TickBbscManagedCycleReceipt();
    assert(Core::Bbsc::Party::LoadManagedCycleEpoch(
        BbscPartyHomePath(cycle_lookup), cycle_lookup, ec)->stage ==
        Core::Bbsc::Party::ManagedCycleStage::Stable);
    TickBbscManagedReturnSaveReceipt(); // fresh stable tagged key + exact post-restore receipt
    auto cycle7_completed = Core::Bbsc::Party::LoadJournal(managed_home_path, managed_key, ec);
    assert(cycle7_completed && cycle7_completed->phase == Core::Bbsc::Party::Phase::Complete &&
           cycle7_completed->save_generation == mock_receipt_generation &&
           !bbsc_managed_return_save_tracker.Armed());
    std::puts("PASS: tagged cycle-7 SerializeEnd, ABI3 receipt, epoch stabilization and managed return complete in callback order");
    cycle7_completed.reset();
    WriteValue<s32>(address(slots), 0x68, 0);
    WriteValue<s32>(address(bytes), 0x254, 0);
    WriteValue<u64>(address(local), 0x690, 2723151752040083ULL);
    WriteValue<u64>(address(bytes), 0x24C, 2723151752040083ULL);
    save();
    mock_receipt_generation = 12;
    mock_receipt_sha = "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390";
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
    TestNativeFlagCallbackHookCompatibility();
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
    TestNativeEventFlagProof();
    TestNativeCycleIncrementEvidence();
    TestNativeCycleAppliedEvidence();
    TestNativeCycleHookReplay();
    TestManagedCycleHookCoverage();
    TestPartyInbox();
    TestPartyPeerSelectionRefresh();
    TestSingleNativeJoinLoad();
    TestPartingBlankRow();
}
