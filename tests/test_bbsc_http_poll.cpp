// SPDX-License-Identifier: GPL-2.0-or-later
#include "../gpu/bbnet/shadps4/core/libraries/network/http.cpp"
#include <cassert>
#include <cstdio>

namespace Common::Log {
bool ShouldLog(Level) { return false; }
void Write(std::string_view, Level, const char*, int, std::string&&) {}
}
namespace Core::Bloodborne {
std::optional<std::string> GetSeamlessHostPlacementHeader() { return {}; }
std::optional<std::uint32_t> GetBbscConnectFirstAreaId() { return {}; }
bool SetSeamlessHostPlacementHeader(std::string_view) { return true; }
void ClearSeamlessHostPlacementHeader() {}
}
namespace Core::Debugger { s32 GetCurrentPid() { return 0; } }
namespace Common::FS {
const std::filesystem::path& GetUserPath(PathType) {
    static const std::filesystem::path path{"/tmp/bbsc-http-poll-no-profile"};
    return path;
}
std::string PathToUTF8String(const std::filesystem::path& path) { return path.string(); }
}

static void TestHttpWorker() {
    using namespace Libraries::Http;
    httplib::Server server;
    std::atomic<int> calls{0};
    std::atomic<bool> deliver{true};
    server.Post("/summon_messenger/create", [](const auto&, auto& res) {
        res.set_content(
            R"({"MessageId":"SummonDataCreateResponse","ResKind":0,"SessionId":"own-session"})",
            "application/json");
    });
    server.Post("/summon_messenger/get", [&](const auto& req, auto& res) {
        const int count = ++calls;
        const auto body = nlohmann::json::parse(req.body);
        if (deliver && count == 2) {
            assert(body["SummonTypeList"].size() == 1);
            assert(body["SummonTypeList"][0]["SummonType"] == 0);
            res.set_content(
                R"({"MessageId":"SummonDataGetListResponse","ResKind":0,"SummonDataList":[{"SessionId":"candidate-session","UserId":222,"CharaId":8,"SummonType":0,"SummonWord":"fixture"}]})",
                "application/json");
        } else {
            res.set_content(
                R"({"MessageId":"SummonDataGetListResponse","ResKind":0,"SummonDataList":[]})",
                "application/json");
        }
    });
    server.Post("/summon_messenger/request", [](const auto&, auto& res) {
        res.set_content(R"({"MessageId":"SummonDataSummonResponse","ResKind":0})",
                        "application/json");
    });
    const int port = server.bind_to_any_port("127.0.0.1");
    assert(port > 0);
    std::thread serving([&] { server.listen_after_bind(); });
    const std::string url = "http://127.0.0.1:" + std::to_string(port);
    setenv("BB_ONLINE", "1", 1);
    setenv("BB_SHADNET_WEBAPI", url.c_str(), 1);
    setenv("SHADPS4_BLOODBORNE_SEAMLESS_COOP", "1", 1);
    setenv("SHADPS4_CAPTURE_BLOODBORNE_SUMMON", "0", 1);
    setenv("SHADPS4_BBSC_NO_PLACEMENT_HEADER", "1", 1);
    setenv("SHADPS4_BBSC_NO_FAST_SIGN", "0", 1);
    const int ctx = sceHttpInit(1, 1, 0x100000);
    const int tmpl = sceHttpCreateTemplate(ctx, "regression", 1, 0);
    const int conn = sceHttpCreateConnectionWithURL(tmpl, url.c_str(), true);
    assert(ctx > 0 && tmpl > 0 && conn > 0);
    const auto send_to = [&](std::string_view path, std::string_view body) {
        const std::string path_string{path};
        const int req = sceHttpCreateRequest(conn, ORBIS_HTTP_METHOD_POST,
                                            path_string.c_str(), body.size());
        assert(req > 0);
        assert(sceHttpSendRequest(req, body.data(), body.size()) == 0);
        return req;
    };
    const auto send = [&](std::string_view body) {
        return send_to("/summon_messenger/get", body);
    };
    const auto complete = [&](int req) {
        int status = 0;
        assert(sceHttpGetStatusCode(req, &status) == 0 && status == 200);
    };
    const auto read_response = [&](int req) {
        char response[512]{};
        const int size = sceHttpReadData(req, response, sizeof(response));
        assert(size > 0);
        return nlohmann::json::parse(response, response + size);
    };
    const std::string mixed = R"({"SummonWord":"fixture","SummonTypeList":[{"SummonType":0},{"SummonType":2}]})";
    const std::string own_sign =
        R"({"SessionId":"own-session","UserId":111,"CharaId":7,"SummonType":0,"SummonWord":"fixture"})";
    const int create_req = send_to("/summon_messenger/create", own_sign);
    complete(create_req);
    assert(GetBbscSummonObservationSnapshot().own_sign.valid);
    assert(read_response(create_req)["MessageId"] == "SummonDataCreateResponse");
    assert(sceHttpDeleteRequest(create_req) == 0);
    assert(GetBbscSummonObservationSnapshot().own_sign.valid);
    const int req = send(mixed);
    complete(req);
    assert(calls == 2); // real worker retries the empty response, then stops at the candidate
    assert(read_response(req)["SummonDataList"].size() == 1);
    assert(sceHttpDeleteRequest(req) == 0);
    const auto claim_body =
        R"({"SessionId":"own-session","UserId":111,"CharaId":7,"TargetUserId":222,"TargetCharaId":8})";
    const int claim_req = send_to("/summon_messenger/request", claim_body);
    complete(claim_req);
    assert(read_response(claim_req)["MessageId"] == "SummonDataSummonResponse");
    assert(sceHttpDeleteRequest(claim_req) == 0);
    const auto delivered_claim = GetBbscSummonObservationSnapshot().claim;
    assert(delivered_claim.valid && delivered_claim.matched_unique_candidate &&
           delivered_claim.candidate.session_id == "candidate-session");
    deliver = false;
    setenv("SHADPS4_BBSC_NO_FAST_SIGN", "1", 1);
    complete(send(mixed));
    assert(calls == 3);
    setenv("SHADPS4_BBSC_NO_FAST_SIGN", "0", 1);
    complete(send(R"({"SummonWord":"","SummonTypeList":[{"SummonType":0}]})"));
    assert(calls == 4);
    complete(send(R"({"SummonWord":"fixture","SummonTypeList":[{"SummonType":2}]})"));
    assert(calls == 5);
    const int aborted = send(mixed);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (calls < 6 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(calls == 6);
    std::shared_ptr<HttpRequest> pending;
    {
        std::lock_guard lock(g_state.m_mutex);
        pending = g_state.requests.at(aborted);
    }
    assert(sceHttpAbortRequest(aborted) == 0);
    while (pending.use_count() > 2 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(pending.use_count() == 2 && calls == 6); // abort wakes the wait; no later extra poll

    const int pending_delete_req = send(mixed);
    const auto delete_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (calls < 7 && std::chrono::steady_clock::now() < delete_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(calls == 7);
    {
        std::lock_guard lock(g_state.m_mutex);
        pending = g_state.requests.at(pending_delete_req);
    }
    assert(sceHttpDeleteRequest(pending_delete_req) == 0);
    while (pending.use_count() > 2 && std::chrono::steady_clock::now() < delete_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(pending.use_count() == 2 && calls == 7);
    const int claim_after_delete = send_to("/summon_messenger/request", claim_body);
    complete(claim_after_delete);
    assert(read_response(claim_after_delete)["ResKind"] == 0);
    assert(sceHttpDeleteRequest(claim_after_delete) == 0);
    assert(!GetBbscSummonObservationSnapshot().claim.valid);
    assert(sceHttpTerm(ctx) == 0);
    server.stop();
    serving.join();
    std::puts("PASS: real HTTP worker polls co-op only, delivers candidate, honors opt-out/abort");
}

static Libraries::Http::SendRequestPlan ObserverPlan(std::string_view path,
                                                     std::string_view body) {
    Libraries::Http::SendRequestPlan plan;
    plan.path = path;
    plan.body.assign(body.begin(), body.end());
    plan.headers.emplace_back("X-ShadPS4-Bloodborne-Seamless", "1");
    return plan;
}

static Libraries::Http::HttpResponse ObserverResponse(std::string_view body, int status = 200) {
    Libraries::Http::HttpResponse response;
    response.status_code = status;
    response.body.assign(body.begin(), body.end());
    return response;
}

static void TestBbscSummonObserver() {
    using namespace Libraries::Http;
    BbscSummonObserver observer;
    const std::array<u8, 16> challenge = {0, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15};
    const std::array<u8, 16> other_challenge = {1, 1, 2, 3, 4, 5, 6, 7,
                                                8, 9, 10, 11, 12, 13, 14, 15};
    constexpr int context = 31;
    const auto successful_create = [&] {
        auto plan = ObserverPlan("/summon_messenger/create",
                                 R"({"SessionId":"own-session","UserId":111,"CharaId":7,"SummonType":0,"SummonWord":"same-word"})");
        const auto ticket = observer.Begin(plan, context);
        observer.Complete(ticket, plan,
                          ObserverResponse(R"({"MessageId":"SummonDataCreateResponse","ResKind":0})"),
                          0);
        return ticket;
    };
    const auto search_plan = [](std::string_view word) {
        std::string body = R"({"SummonWord":")";
        body += word;
        body += R"(","SummonTypeList":[{"SummonType":0}]})";
        return ObserverPlan("/summon_messenger/get", body);
    };
    const auto candidate_response = [](std::string_view rows) {
        std::string body = R"({"MessageId":"SummonDataGetListResponse","ResKind":0,"SummonDataList":[)";
        body += rows;
        body += "]}";
        return ObserverResponse(body);
    };
    const auto candidate_row = [](std::string_view session, int user, int chara,
                                  std::string_view word, int type = 0) {
        std::string row = R"({"SessionId":")";
        row += session;
        row += R"(","UserId":)" + std::to_string(user);
        row += R"(,"CharaId":)" + std::to_string(chara);
        row += R"(,"SummonType":)" + std::to_string(type);
        row += R"(,"SummonWord":")";
        row += word;
        row += R"("})";
        return row;
    };
    const auto run_search = [&](const SendRequestPlan& plan, const std::string& rows) {
        const auto ticket = observer.Begin(plan, context);
        observer.Complete(ticket, plan, candidate_response(rows), 0);
        return ticket;
    };
    const auto claim_plan = [](int target_user, int target_chara) {
        std::string body = R"({"SessionId":"own-session","UserId":111,"CharaId":7,"TargetUserId":)";
        body += std::to_string(target_user);
        body += R"(,"TargetCharaId":)";
        body += std::to_string(target_chara);
        body += "}";
        return ObserverPlan("/summon_messenger/request", body);
    };
    const auto finish_claim = [&](const auto& ticket, const auto& plan, bool success = true) {
        const auto response = success
            ? ObserverResponse(R"({"MessageId":"SummonDataSummonResponse","ResKind":0})")
            : ObserverResponse(R"({"MessageId":"SummonDataSummonResponse","ResKind":1})");
        observer.Complete(ticket, plan, response, 0);
    };

    observer.InvalidateContext(context);
    successful_create();
    const std::array<u8, 32> empty_proof{};
    const auto own_only = observer.Proofs(challenge);
    assert(own_only && own_only->selected == empty_proof);
    const auto search = search_plan("same-word");
    run_search(search, candidate_row("candidate-a", 222, 8, "same-word"));
    auto claim = claim_plan(222, 8);
    auto claim_ticket = observer.Begin(claim, context);
    finish_claim(claim_ticket, claim);
    auto snapshot = observer.Snapshot();
    assert(snapshot.own_sign.valid && snapshot.own_sign.session_id == "own-session");
    assert(snapshot.claim.valid && snapshot.claim.candidate.session_id == "candidate-a");
    assert(snapshot.claim.password_matches_own_sign && snapshot.claim.token != 0);
    const auto proofs = observer.Proofs(challenge);
    assert(proofs && proofs->observation.own_sign.session_id == "own-session" &&
           proofs->observation.claim.candidate.session_id == "candidate-a");
    // Independent Python hmac/struct fixture: key same-word, nonce 00..0f, little-endian fields.
    const std::array<u8, 32> expected_own = {
        0xed, 0x8e, 0xf3, 0x6f, 0xc2, 0x44, 0x04, 0x21, 0x97, 0xd8, 0x56, 0x9c, 0xf1, 0x60, 0x0b, 0x04,
        0x3c, 0x9f, 0x48, 0x1f, 0xd9, 0xfb, 0x7e, 0x7e, 0xe7, 0xa0, 0xa1, 0x56, 0x17, 0x23, 0xfd, 0xc2};
    const std::array<u8, 32> expected_selected = {
        0x31, 0xbe, 0x3d, 0x36, 0x5e, 0x9d, 0x47, 0x6f, 0xa7, 0xd4, 0x15, 0xe6, 0x21, 0x05, 0x27, 0x8f,
        0xcc, 0xb0, 0x00, 0x34, 0x07, 0xf5, 0xac, 0xa3, 0xc0, 0xe2, 0x08, 0xa9, 0xb4, 0x86, 0xc1, 0x10};
    assert(proofs->own == expected_own && proofs->selected == expected_selected);
    const auto other_nonce_proofs = observer.Proofs(other_challenge);
    assert(other_nonce_proofs && other_nonce_proofs->own != proofs->own &&
           other_nonce_proofs->selected != proofs->selected);

    BbscSummonObserver candidate_observer;
    auto candidate_sign = ObserverPlan(
        "/summon_messenger/create",
        R"({"SessionId":"candidate-a","UserId":222,"CharaId":8,"SummonType":0,"SummonWord":"same-word"})");
    auto candidate_ticket = candidate_observer.Begin(candidate_sign, context);
    candidate_observer.Complete(
        candidate_ticket, candidate_sign,
        ObserverResponse(R"({"MessageId":"SummonDataCreateResponse","ResKind":0})"), 0);
    const auto candidate_proof = candidate_observer.Proofs(challenge);
    assert(candidate_proof && candidate_proof->own == proofs->selected);

    BbscSummonObserver wrong_word_observer;
    auto wrong_word_sign = ObserverPlan(
        "/summon_messenger/create",
        R"({"SessionId":"own-session","UserId":111,"CharaId":7,"SummonType":0,"SummonWord":"other-word"})");
    const auto wrong_word_ticket = wrong_word_observer.Begin(wrong_word_sign, context);
    wrong_word_observer.Complete(
        wrong_word_ticket, wrong_word_sign,
        ObserverResponse(R"({"MessageId":"SummonDataCreateResponse","ResKind":0})"), 0);
    const auto wrong_word_proof = wrong_word_observer.Proofs(challenge);
    assert(wrong_word_proof && wrong_word_proof->own != proofs->own);
    const std::array<u8, 16> zero_challenge{};
    assert(!observer.Proofs(zero_challenge));
    assert(!BbscSummonObserver{}.Proofs(challenge));

    // Duplicate target identities are ambiguous even when their sessions differ.
    run_search(search, candidate_row("candidate-a", 222, 8, "same-word") + "," +
                           candidate_row("candidate-b", 222, 8, "same-word"));
    claim_ticket = observer.Begin(claim, context);
    finish_claim(claim_ticket, claim);
    assert(!observer.Snapshot().claim.valid);

    // A wrong password or invasion row cannot become a co-op selection.
    run_search(search, candidate_row("wrong-word", 222, 8, "other-word"));
    claim_ticket = observer.Begin(claim, context);
    finish_claim(claim_ticket, claim);
    assert(!observer.Snapshot().claim.valid);
    run_search(search, candidate_row("invasion", 222, 8, "same-word", 2));
    claim_ticket = observer.Begin(claim, context);
    finish_claim(claim_ticket, claim);
    assert(!observer.Snapshot().claim.valid);

    // A newer search invalidates an older one; a late old response cannot win.
    const auto old_search = search_plan("same-word");
    const auto old_ticket = observer.Begin(old_search, context);
    const auto new_search = search_plan("same-word");
    const auto new_ticket = observer.Begin(new_search, context);
    observer.Complete(new_ticket, new_search,
                      candidate_response(candidate_row("newer", 222, 8, "same-word")), 0);
    observer.Complete(old_ticket, old_search,
                      candidate_response(candidate_row("older", 222, 8, "same-word")), 0);
    const auto newer_ordered = observer.Begin(new_search, context, 102);
    const auto older_delayed = observer.Begin(old_search, context, 101);
    assert(newer_ordered.kind == BbscSummonObserverKind::Search &&
           older_delayed.kind == BbscSummonObserverKind::None);
    observer.Complete(newer_ordered, new_search,
                      candidate_response(candidate_row("ordered-new", 222, 8, "same-word")), 0);
    claim_ticket = observer.Begin(claim, context);
    const auto previous_attempt = claim_ticket.token;
    finish_claim(claim_ticket, claim);
    auto older_claim = claim_plan(222, 8);
    auto older_ticket = observer.Begin(older_claim, context);
    auto latest_claim = claim_plan(222, 8);
    auto latest_ticket = observer.Begin(latest_claim, context);
    finish_claim(older_ticket, older_claim);
    finish_claim(latest_ticket, latest_claim);
    snapshot = observer.Snapshot();
    assert(latest_ticket.token > previous_attempt && snapshot.claim.token == latest_ticket.token);
    assert(snapshot.claim.candidate.session_id == "ordered-new");

    // Failed claims and explicit cancellation leave no usable selected attempt.
    claim_ticket = observer.Begin(claim, context);
    finish_claim(claim_ticket, claim, false);
    assert(!observer.Snapshot().claim.valid);
    claim_ticket = observer.Begin(claim, context);
    observer.Cancel(claim_ticket);
    assert(!observer.Snapshot().claim.valid);

    // Failed creates, malformed/oversized fields, and context destruction fail closed.
    auto bad_create = ObserverPlan("/summon_messenger/create",
                                   R"({"SessionId":"own-session","UserId":111,"CharaId":7,"SummonType":0,"SummonWord":"same-word"})");
    auto bad_ticket = observer.Begin(bad_create, context);
    observer.Complete(bad_ticket, bad_create,
                      ObserverResponse(R"({"MessageId":"SummonDataCreateResponse","ResKind":1})"),
                      0);
    assert(!observer.Snapshot().own_sign.valid);
    bad_ticket = observer.Begin(bad_create, context);
    observer.Complete(bad_ticket, bad_create,
                      ObserverResponse(R"({"MessageId":"SummonDataCreateResponse","ResKind":0})", 500),
                      0);
    assert(!observer.Snapshot().own_sign.valid);
    successful_create();
    auto malformed_search = search_plan("same-word");
    auto malformed_ticket = observer.Begin(malformed_search, context);
    observer.Complete(malformed_ticket, malformed_search,
                      candidate_response(R"({"SessionId":"missing-fields"})"), 0);
    assert(!observer.Snapshot().claim.valid);
    const auto oversized = std::string(256, 'x');
    malformed_ticket = observer.Begin(malformed_search, context);
    observer.Complete(malformed_ticket, malformed_search,
                      candidate_response(candidate_row(oversized, 222, 8, "same-word")), 0);
    assert(!observer.Snapshot().claim.valid);
    observer.InvalidateContext(context);
    snapshot = observer.Snapshot();
    assert(!snapshot.own_sign.valid && !snapshot.claim.valid);
    assert(!observer.Proofs(challenge));
}

int main() {
    using namespace Libraries::Http;
    const auto request = [](std::string_view body) {
        SendRequestPlan p;
        p.path = "/summon_messenger/get?user_id=123";
        p.body.assign(body.begin(), body.end());
        return p;
    };
    auto mixed = request(R"({"SummonWord":"fixture","SummonTypeList":[{"SummonType":0,"GetLimitCount":4},{"SummonType":2}],"MessageId":"find"})");
    assert(PrepareBbscFastSearch(mixed));
    const auto filtered = nlohmann::json::parse(mixed.body);
    assert(filtered["SummonTypeList"].size() == 1);
    assert(filtered["SummonTypeList"][0]["SummonType"] == 0);
    assert(filtered["SummonTypeList"][0]["GetLimitCount"] == 4);
    assert(filtered["MessageId"] == "find");
    for (auto body : {R"({"SummonWord":"","SummonTypeList":[{"SummonType":0}]})",
                      R"({"SummonWord":"fixture","SummonTypeList":[{"SummonType":2}]})",
                      R"({"SummonWord":123,"SummonTypeList":[{"SummonType":0}]})",
                      R"({"SummonWord":"fixture","SummonTypeList":[{"SummonType":"0"}]})",
                      R"({"SummonWord":"fixture","SummonTypeList":{}})", "broken"}) {
        auto p = request(body);
        const auto original = p.body;
        assert(!PrepareBbscFastSearch(p));
        assert(p.body == original);
    }
    HttpResponse res;
    const auto response = [&res](std::string_view body) {
        res.body.assign(body.begin(), body.end());
        return BbscFastSearchPending(res);
    };
    assert(response(R"({"SummonDataList":[]})"));
    assert(!response(R"({"SummonDataList":[{"SessionId":"fixture"}]})"));
    assert(!response(R"({"SummonDataList":{}})"));
    assert(!response(R"({"ErrorCode":1})"));
    assert(!response("broken"));
    std::puts("PASS: password co-op search polling, mixed-search filtering, stop on candidate/error");
    TestBbscSummonObserver();
    std::puts("PASS: bounded BBSC own-sign and selected-claim observation");
    TestHttpWorker();
    const std::array<u8, 16> challenge = {0, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15};
    assert(!GetBbscSummonCapabilityProofs(challenge));
}
