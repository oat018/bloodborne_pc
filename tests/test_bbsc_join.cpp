// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual placement cache, including anonymous game-thread state.
#include "../gpu/bbnet/shadps4/core/bloodborne_re.cpp"
#include <cassert>
#include <cstdio>
#include <sys/mman.h>

extern "C" int runtime_memory_vma_info(uintptr_t, int*, int*, uintptr_t*) { return 0; }
static std::string receipt_query;
static u64 mock_receipt_generation = 12;
static std::string mock_receipt_sha = "784549a4d1d963aff045cb64b9a38ceaf0b358993310b8dad3d61fb131cbc390";
int32_t Core::ReadRuntimeSaveReceipt(const char* path, BbNetSaveReceipt* out) {
    receipt_query = path;
    *out = {};
    if (receipt_query != "/savedata0/userdata0001") return ENOENT;
    out->generation = mock_receipt_generation;
    out->bytes = 0x140000;
    std::strncpy(out->sha256, mock_receipt_sha.c_str(), sizeof(out->sha256) - 1);
    const char* save_dir = std::getenv("BBSC_SAVE_DIR");
    if (!save_dir) return ENOENT;
    const auto save_path = (std::filesystem::path{save_dir} / "userdata0001").string();
    std::strncpy(out->path, save_path.c_str(), sizeof(out->path) - 1);
    return 0;
}
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
    const auto address = [](auto& data) { return reinterpret_cast<u64>(data.data()); };
    image_base = address(image);
    MemoryPatcher::g_eboot_image_size = image.size();
    WriteValue<u64>(image_base, GlobalStatePointerOffset, address(global));
    WriteValue<u64>(image_base, PlayerSlotManagerPointerOffset, address(slots));
    WriteValue<u64>(image_base, MatchingStatePointerOffset, address(matching));
    WriteValue<u64>(address(slots), 8, address(local));
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
    std::array<u8, 0x70> local_state{}, local_world{}, model{};
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
    auto home = PrepareBbscNativePartyHome({4, 5}, ec);
    assert(home && !ec);
    assert(home->record.key == original_key && home->record.admission == 0);
    assert(home->record.phase == Core::Bbsc::Party::Phase::Prepared);
    assert(home->record.origin.map == 0x18010000 && home->record.origin.x == 10);
    assert(home->record.fallback.map == 0x18010000 && home->record.fallback.x == 100);
    assert(home->record.fallback.lamp == 2412950);
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
    TestNativeCycleAppliedEvidence();
    TestNativeCycleHookReplay();
    TestPartyInbox();
    TestSingleNativeJoinLoad();
    TestPartingBlankRow();
}
