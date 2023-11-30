#include "etcdv3_call.h"
#include "etcdv3_mvcckv.h"
#include "etcdv3_mvcckv_impl.h"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <spdlog/spdlog.h>

namespace etcdv3 {

static int64_t NextId()
{
    static std::atomic_int_fast64_t global_id{0};
    return global_id.fetch_add(1);
}

Callbase::Callbase()
    : id(NextId())
{}

static std::string to_string(WatchStatus state)
{
    switch (state)
    {
    case WatchStatus::initializing:
        return "initializing";
    case WatchStatus::writing:
        return "writing";
    case WatchStatus::reading:
        return "reading";
    case WatchStatus::connected:
        return "connected";
    case WatchStatus::canceling:
        return "canceling";
    case WatchStatus::cancelled:
        return "cancelled";
    }
    return "unknown";
}

void etcdv3::AsyncWatchCall::OnResponse()
{
    std::lock_guard<std::mutex> lock(state_mutex);
    spdlog::debug("on async watch reponse, state:{}", to_string(watcher_status));
    switch (watcher_status)
    {
    case WatchStatus::initializing: {
        watcher_status = WatchStatus::writing;
        spdlog::debug("watcher id: {} initializing", watch_id);
        etcdserverpb::WatchRequest request;
        request.mutable_create_request()->set_watch_id(watch_id);
        request.mutable_create_request()->set_key(key);
        request.mutable_create_request()->set_range_end(range_end);
        request.mutable_create_request()->set_prev_kv(true);
        stream->Write(request, this);
    }
    break;
    case WatchStatus::writing:
        watcher_status = WatchStatus::reading;
        spdlog::debug("watcher id: {} write stream created", watch_id);
        stream->Read(reply.get(), this);
        break;
    case WatchStatus::reading:
        watcher_status = WatchStatus::connected;
        spdlog::debug(
            "watcher id: {} read stream created, bidirectional stream connected", watch_id);
        stream->Read(reply.get(), this);
        break;
    case WatchStatus::connected:
        spdlog::debug("watcher id: {} === read response, created:{}, canceled:{}", watch_id,
            reply->created(), reply->canceled());
        if (reply->created())
        {
            spdlog::debug("watcher id: {} created successfully", watch_id);
        }
        else
        {
            spdlog::debug("watcher id: {} >>> got event response", watch_id);
            EventRangeImpl range(*reply);
            if (callback)
            {
                spdlog::debug("watcher id: {} calling callback", watch_id);
                callback(range);
            }
        }
        spdlog::debug("watcher id: {} begin to read next reponse", watch_id);
        stream->Read(reply.get(), this);
        break;
    case WatchStatus::canceling:
        if (reply->canceled())
        {
            watcher_status = WatchStatus::cancelled;
            spdlog::debug("watcher id: {} --- received a cancel confirmation", watch_id);
            stream->Finish(status.get(), this);
        }
        else
        {
            spdlog::debug(
                "watcher id: {} received something during canceling, created:{}, canceled:{}",
                watch_id, reply->created(), reply->canceled());
        }
        break;
    default:
        break;
    }
    spdlog::debug("leaving on watch response, new state: {}", to_string(watcher_status));
}

} // namespace etcdv3
