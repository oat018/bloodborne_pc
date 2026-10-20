// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bloodborne_re.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "../gpu/bbnet/shadps4/core/libraries/np/np_matching2/np_matching2_mm.cpp"
#include "common/logging/log.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>

using namespace Libraries::Np::NpMatching2;

namespace {
std::atomic<u64> next_cleanup_pkt = 9000;

struct CallbackState {
    std::mutex mutex;
    std::condition_variable cv;
    int calls = 0;
    int error = 0;
    const void* data = nullptr;
};

void PS4_SYSV_ABI RequestCallback(OrbisNpMatching2ContextId, OrbisNpMatching2RequestId,
                                 OrbisNpMatching2Event, int error, const void* data,
                                 void* userdata) {
    auto& state = *static_cast<CallbackState*>(userdata);
    {
        std::lock_guard lock(state.mutex);
        ++state.calls;
        state.error = error;
        state.data = data;
    }
    state.cv.notify_all();
}

bool WaitForCallback(CallbackState& state) {
    std::unique_lock lock(state.mutex);
    return state.cv.wait_for(lock, std::chrono::seconds(2), [&] { return state.calls != 0; });
}

void RoomMessageCallback(OrbisNpMatching2ContextId, OrbisNpMatching2RoomId,
                         OrbisNpMatching2RoomMemberId, OrbisNpMatching2Event, const void*,
                         void* userdata) {
    auto& state = *static_cast<CallbackState*>(userdata);
    {
        std::lock_guard lock(state.mutex);
        ++state.calls;
    }
    state.cv.notify_all();
}

void CompleteCurrentCleanup(OrbisNpMatching2ContextId context,
                           OrbisNpMatching2RoomId room) {
    const auto it = g_state.admission.find(context);
    assert(it != g_state.admission.end() && it->second.cleanup_pending);
    assert(it->second.cleanup_nonce != 0 && it->second.ticket.room == room);
    shadnet::LeaveRoomReply reply;
    reply.set_room_id(room);
    OnMatchingReply(ShadNet::CommandType::LeaveRoom, next_cleanup_pkt.load(),
                    ShadNet::ErrorType::NoError, MakeProtoPayload(reply));
    assert(!HasBbscAdmissionPending(context));
}

void SetLiveRoom(ContextObject& ctx) {
    ctx.room_id = 55;
    ctx.my_member_id = 1;
    RoomCache room{};
    room.room_id = ctx.room_id;
    room.signaling_main_member = 2;
    MemberCache owner{};
    owner.member_id = 2;
    SetNpId(owner.np_id, "host-online-id");
    room.members.emplace(owner.member_id, owner);
    ctx.room_cache.emplace(room.room_id, std::move(room));
}
} // namespace

namespace Common {
void SetCurrentThreadName(const char*) {}
} // namespace Common

namespace Common::Log {
bool ShouldLog(Level) {
    return false;
}
void Write(std::string_view, Level, const char*, int, std::string&&) {}
} // namespace Common::Log

namespace Core::Bloodborne {
void OnBbscPartyRoomState(BbscPartyRoom) {}
} // namespace Core::Bloodborne

namespace Libraries::Net {
u16 PS4_SYSV_ABI sceNetHtons(u16 value) {
    return static_cast<u16>((value << 8) | (value >> 8));
}
} // namespace Libraries::Net

namespace Libraries::Np::NpCommon {
u32 PS4_SYSV_ABI sceNpJoinThread(Libraries::Kernel::PthreadT, void**) {
    return ORBIS_OK;
}
} // namespace Libraries::Np::NpCommon

namespace Libraries::Np::NpMatching2 {
void StartMatching2SignalingForRoomPeers(ContextObject&, OrbisNpMatching2RoomId) {}
void QueueMatching2DeadForRoomPeers(ContextObject&, OrbisNpMatching2RoomId, s32) {}
} // namespace Libraries::Np::NpMatching2

namespace ShadNet {
bool ShadNetClient::IsAuthenticated() const {
    return true;
}
u64 ShadNetClient::SubmitRequest(CommandType, const std::vector<u8>&) {
    return ++next_cleanup_pkt;
}
} // namespace ShadNet

int main() {
    OrbisNpMatching2ContextId context = 0;
    auto& manager = ContextManager::Instance();
    assert(manager.CreateContext(nullptr, 0, &context) == ORBIS_OK);
    assert(manager.Start(context) == ORBIS_OK);
    ContextObject* ctx = manager.Get(context);
    SetLiveRoom(*ctx);
    CallbackState room_message;
    ctx->room_message_callback = RoomMessageCallback;
    ctx->room_message_callback_arg = &room_message;

    g_state.dispatch_running.store(true);
    g_mm.client = std::shared_ptr<ShadNet::ShadNetClient>(
        reinterpret_cast<ShadNet::ShadNetClient*>(uintptr_t{1}), [](ShadNet::ShadNetClient*) {});
    std::thread dispatcher([] { EventDispatcherThreadMain(nullptr); });

    OrbisNpMatching2ContextId stale_context = 0;
    assert(manager.CreateContext(nullptr, 0, &stale_context) == ORBIS_OK);
    assert(manager.Start(stale_context) == ORBIS_OK);
    constexpr OrbisNpMatching2RequestId stale_request = 61;
    assert(ArmBbscAdmission(stale_context, stale_request));
    const u64 stale_incarnation = manager.Get(stale_context)->incarnation;
    assert(manager.Destroy(stale_context));
    std::array<OrbisNpMatching2ContextId, ContextManager::kMaxContexts - 2> occupied_contexts{};
    for (auto& occupied_context : occupied_contexts) {
        assert(manager.CreateContext(nullptr, 0, &occupied_context) == ORBIS_OK);
    }
    OrbisNpMatching2ContextId reused_context = 0;
    assert(manager.CreateContext(nullptr, 0, &reused_context) == ORBIS_OK);
    assert(reused_context == stale_context);
    assert(manager.Start(reused_context) == ORBIS_OK);
    constexpr u64 stale_join_packet = 7001;
    PendingRequest stale_join{};
    stale_join.ctx_id = stale_context;
    stale_join.ctx_incarnation = stale_incarnation;
    stale_join.admission_armed = true;
    stale_join.req_id = stale_request;
    stale_join.req_event = ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM;
    {
        std::lock_guard lock(g_mm.pending_mutex);
        g_mm.pending.emplace(stale_join_packet, stale_join);
    }
    OnMatchingReply(ShadNet::CommandType::JoinRoom, stale_join_packet,
                    ShadNet::ErrorType::RoomMissing, {});
    assert(!HasBbscAdmissionPending(reused_context));
    assert(ArmBbscAdmission(reused_context, stale_request + 1));
    CancelBbscAdmissionRequest(reused_context, stale_request + 1);
    assert(manager.Destroy(reused_context));
    for (const auto occupied_context : occupied_contexts) assert(manager.Destroy(occupied_context));

    constexpr OrbisNpMatching2RequestId accepted_request = 71;
    CallbackState accepted;
    int response = 42;
    assert(ArmBbscAdmission(context, accepted_request));
    assert(!ArmBbscAdmission(context, accepted_request + 1));
    assert(HoldBbscAdmissionResponse(*ctx, accepted_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                                     RequestCallback, &accepted, &response,
                                     std::make_shared<CallbackPayload>()));
    const auto tickets = GetPendingBbscAdmissionTickets();
    assert(tickets.size() == 1 && tickets.front().room == 55);
    assert(tickets.front().owner.data[0] == 'h');
    PendingEvent room_message_event{};
    room_message_event.type = PendingEvent::ROOM_MESSAGE_CB;
    room_message_event.ctx_id = context;
    room_message_event.room_id = 55;
    room_message_event.src_member_id = 2;
    room_message_event.message_data = &response;
    ScheduleEvent(std::move(room_message_event));
    assert(WaitForCallback(room_message));
    {
        std::lock_guard lock(accepted.mutex);
        assert(accepted.calls == 0);
    }
    auto altered_ticket = tickets.front();
    altered_ticket.owner.data[0] ^= 1;
    assert(!ResolveBbscAdmission(altered_ticket, true));
    assert(ResolveBbscAdmission(tickets.front(), true));
    assert(!ResolveBbscAdmission(tickets.front(), false));
    assert(WaitForCallback(accepted));
    assert(accepted.calls == 1 && accepted.error == ORBIS_OK && accepted.data == &response);

    constexpr OrbisNpMatching2RequestId rejected_request = 72;
    CallbackState rejected;
    assert(ArmBbscAdmission(context, rejected_request));
    assert(HoldBbscAdmissionResponse(*ctx, rejected_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A,
                                     RequestCallback, &rejected, &response,
                                     std::make_shared<CallbackPayload>()));
    const auto rejected_tickets = GetPendingBbscAdmissionTickets();
    assert(rejected_tickets.size() == 1);
    assert(!ResolveBbscAdmission(rejected_tickets.front(), false));
    assert(WaitForCallback(rejected));
    assert(rejected.calls == 1 && rejected.error == ORBIS_NP_MATCHING2_ERROR_ABORTED &&
           rejected.data == nullptr);
    assert(!ResolveBbscAdmission(rejected_tickets.front(), false));
    assert(rejected.calls == 1);
    assert(HasBbscAdmissionPending(context));
    assert(!ArmBbscAdmission(context, 76));
    const u64 first_cleanup_nonce = g_state.admission.at(context).cleanup_nonce;
    const u64 context_incarnation = ctx->incarnation;
    const u64 cleanup_submit_count = next_cleanup_pkt.load();
    assert(BeginBbscAdmissionCleanup(context, context_incarnation, rejected_request, 55));
    assert(g_state.admission.at(context).cleanup_nonce == first_cleanup_nonce);
    assert(next_cleanup_pkt.load() == cleanup_submit_count);
    CompleteCurrentCleanup(context, 55);
    assert(ctx->room_id == 0 && !ctx->room_cache.contains(55));
    assert(ArmBbscAdmission(context, 76));
    CancelBbscAdmissionRequest(context, 76);
    SetLiveRoom(*ctx);

    constexpr OrbisNpMatching2RequestId changed_owner_request = 73;
    CallbackState changed_owner;
    assert(ArmBbscAdmission(context, changed_owner_request));
    assert(HoldBbscAdmissionResponse(*ctx, changed_owner_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                                     RequestCallback, &changed_owner, &response,
                                     std::make_shared<CallbackPayload>()));
    const auto changed_owner_ticket = GetPendingBbscAdmissionTickets().front();
    SetNpId(ctx->room_cache.at(55).members.at(2).np_id, "different-host");
    assert(!ResolveBbscAdmission(changed_owner_ticket, true));
    assert(WaitForCallback(changed_owner));
    assert(changed_owner.calls == 1 && changed_owner.error == ORBIS_NP_MATCHING2_ERROR_ABORTED &&
           changed_owner.data == nullptr);
    assert(HasBbscAdmissionPending(context));
    assert(!CompleteBbscAdmissionCleanup(context, context_incarnation, rejected_request, 55,
                                          first_cleanup_nonce, true));
    assert(ctx->room_id == 55 && ctx->room_cache.contains(55));
    CompleteCurrentCleanup(context, 55);
    SetLiveRoom(*ctx);
    SetNpId(ctx->room_cache.at(55).members.at(2).np_id, "host-online-id");

    constexpr OrbisNpMatching2RequestId timeout_request = 74;
    CallbackState timed_out;
    assert(ArmBbscAdmission(context, timeout_request));
    assert(HoldBbscAdmissionResponse(*ctx, timeout_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                                     RequestCallback, &timed_out, &response,
                                     std::make_shared<CallbackPayload>()));
    auto timeout_ticket = GetPendingBbscAdmissionTickets().front();
    timeout_ticket.deadline = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    {
        std::lock_guard lock(g_state.admission_mutex);
        g_state.admission.at(context).ticket.deadline = timeout_ticket.deadline;
    }
    PendingEvent timeout_event{};
    timeout_event.type = PendingEvent::ADMISSION_TIMEOUT;
    timeout_event.ctx_id = context;
    timeout_event.ctx_incarnation = timeout_ticket.incarnation;
    timeout_event.fire_at = std::chrono::steady_clock::now();
    timeout_event.admission_ticket = timeout_ticket;
    ScheduleEvent(std::move(timeout_event));
    assert(WaitForCallback(timed_out));
    assert(timed_out.calls == 1 && timed_out.error == ORBIS_NP_MATCHING2_ERROR_REQUEST_TIMEOUT &&
           timed_out.data == nullptr);
    assert(HasBbscAdmissionPending(context));
    CompleteCurrentCleanup(context, 55);
    SetLiveRoom(*ctx);

    constexpr OrbisNpMatching2RequestId stopped_request = 75;
    CallbackState stopped;
    assert(ArmBbscAdmission(context, stopped_request));
    assert(HoldBbscAdmissionResponse(*ctx, stopped_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                                     RequestCallback, &stopped, &response,
                                     std::make_shared<CallbackPayload>()));
    const auto stopped_ticket = GetPendingBbscAdmissionTickets().front();
    assert(manager.Stop(context) == ORBIS_OK);
    assert(!ResolveBbscAdmission(stopped_ticket, true));
    assert(GetPendingBbscAdmissionTickets().empty());
    assert(HasBbscAdmissionPending(context));
    CompleteCurrentCleanup(context, 55);
    assert(manager.Start(context) == ORBIS_OK);
    assert(stopped.calls == 0);
    SetLiveRoom(*ctx);

    OrbisNpMatching2ContextId failed_cleanup_context = 0;
    assert(manager.CreateContext(nullptr, 0, &failed_cleanup_context) == ORBIS_OK);
    assert(manager.Start(failed_cleanup_context) == ORBIS_OK);
    ContextObject* failed_ctx = manager.Get(failed_cleanup_context);
    SetLiveRoom(*failed_ctx);
    constexpr OrbisNpMatching2RequestId failed_cleanup_request = 81;
    CallbackState failed_cleanup_callback;
    assert(ArmBbscAdmission(failed_cleanup_context, failed_cleanup_request));
    assert(HoldBbscAdmissionResponse(*failed_ctx, failed_cleanup_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                                     RequestCallback, &failed_cleanup_callback, &response,
                                     std::make_shared<CallbackPayload>()));
    const auto failed_ticket = GetPendingBbscAdmissionTickets().front();
    assert(!ResolveBbscAdmission(failed_ticket, false));
    assert(WaitForCallback(failed_cleanup_callback));
    const u64 failed_packet = next_cleanup_pkt.load();
    OnMatchingReply(ShadNet::CommandType::LeaveRoom, failed_packet,
                    ShadNet::ErrorType::RoomMissing, {});
    assert(g_state.admission.at(failed_cleanup_context).cleanup_failed);
    assert(!ArmBbscAdmission(failed_cleanup_context, failed_cleanup_request + 1));

    manager.Destroy(failed_cleanup_context);

    constexpr OrbisNpMatching2RequestId shutdown_request = 76;
    CallbackState shutdown_callback;
    auto shutdown_payload = std::make_shared<CallbackPayload>();
    std::weak_ptr<CallbackPayload> shutdown_weak = shutdown_payload;
    assert(ArmBbscAdmission(context, shutdown_request));
    assert(HoldBbscAdmissionResponse(*ctx, shutdown_request,
                                     ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                                     RequestCallback, &shutdown_callback, &response,
                                     std::move(shutdown_payload)));
    const auto shutdown_ticket = GetPendingBbscAdmissionTickets().front();
    TermEventDispatcher();
    dispatcher.join();
    assert(!g_state.dispatch_running.load());
    assert(shutdown_weak.expired());
    assert(g_state.pending_events.empty());
    assert(!ResolveBbscAdmission(shutdown_ticket, true));
    assert(shutdown_callback.calls == 0);

    manager.Destroy(context);
    std::puts("PASS: admission hold preserves room messages, resolves once, and gates retries through correlated cleanup");
}
