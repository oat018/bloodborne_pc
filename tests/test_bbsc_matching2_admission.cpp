// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/bloodborne_re.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
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
std::atomic<int> cleanup_calls = 0;

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

namespace Libraries::Np::NpMatching2 {
s32 MmLeaveRoomForAdmission(OrbisNpMatching2ContextId, u64, OrbisNpMatching2RoomId) {
    ++cleanup_calls;
    return ORBIS_OK;
}
} // namespace Libraries::Np::NpMatching2

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
    std::thread dispatcher([] { EventDispatcherThreadMain(nullptr); });

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
    assert(cleanup_calls == 1);

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
    assert(cleanup_calls == 2);
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
    assert(cleanup_calls == 3);

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
    assert(manager.Start(context) == ORBIS_OK);
    assert(stopped.calls == 0);

    manager.Destroy(context);
    g_state.dispatch_running.store(false);
    g_state.queue_cv.notify_all();
    dispatcher.join();
    std::puts("PASS: admission hold preserves room messages, resolves once, and dispatches rejection/timeout cleanup");
}
