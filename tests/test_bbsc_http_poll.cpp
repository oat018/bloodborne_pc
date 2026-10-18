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
    server.Post("/summon_messenger/get", [&](const auto& req, auto& res) {
        const int count = ++calls;
        const auto body = nlohmann::json::parse(req.body);
        if (deliver && count == 2) {
            assert(body["SummonTypeList"].size() == 1);
            assert(body["SummonTypeList"][0]["SummonType"] == 0);
            res.set_content(R"({"SummonDataList":[{"SessionId":"fixture"}]})", "application/json");
        } else {
            res.set_content(R"({"SummonDataList":[]})", "application/json");
        }
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
    const auto send = [&](std::string_view body) {
        const int req = sceHttpCreateRequest(conn, ORBIS_HTTP_METHOD_POST,
                                            "/summon_messenger/get", body.size());
        assert(req > 0);
        assert(sceHttpSendRequest(req, body.data(), body.size()) == 0);
        return req;
    };
    const auto complete = [&](int req) {
        int status = 0;
        assert(sceHttpGetStatusCode(req, &status) == 0 && status == 200);
    };
    const std::string mixed = R"({"SummonWord":"fixture","SummonTypeList":[{"SummonType":0},{"SummonType":2}]})";
    const int req = send(mixed);
    complete(req);
    assert(calls == 2); // real worker retries the empty response, then stops at the candidate
    char data[512]{};
    const int size = sceHttpReadData(req, data, sizeof(data));
    assert(size > 0 && nlohmann::json::parse(data, data + size)["SummonDataList"].size() == 1);
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
    assert(sceHttpTerm(ctx) == 0);
    server.stop();
    serving.join();
    std::puts("PASS: real HTTP worker polls co-op only, delivers candidate, honors opt-out/abort");
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
    TestHttpWorker();
}
