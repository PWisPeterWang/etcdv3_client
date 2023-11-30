#pragma once
#include "etcdv3_response.h"
#include "etcdv3_response_impl.h"
#include <atomic>
#include <cstdint>
#include <etcd/api/rpc.pb.h>
#include <grpcpp/grpcpp.h>
#include <etcd/api/rpc.grpc.pb.h>
#include <future>
#include <grpcpp/impl/codegen/call_op_set.h>
#include <memory>
#include <chrono>
#include <mutex>
#include <spdlog/spdlog.h>

namespace etcdv3 {

using clk = std::chrono::system_clock;

enum class CallType
{
    PUT,
    RANGE,
};

class Callbase
{
public:
    virtual ~Callbase() = default;
    virtual void OnResponse() = 0;
    Callbase();
    int64_t Id() const
    {
        return id;
    }
    virtual bool IsDone() const = 0;

private:
    const int64_t id;
};

class AsyncPutCall final : public Callbase
{
public:
    AsyncPutCall(std::string const &token, std::string const &key, std::string const &value)
    {
        ctx.AddMetadata("token", token);
        request.set_key(key);
        request.set_value(value);
        request.set_prev_kv(true);
    }

    void OnResponse() override
    {
        spdlog::debug("on async put call response");
        if (callback)
        {
            callback(std::make_shared<PutResponse>(status, reply));
        }
        else
        {
            promise.set_value(std::make_shared<PutResponse>(status, reply));
        }
    }

    bool IsDone() const override
    {
        return true;
    }

    etcdserverpb::PutRequest request;
    std::shared_ptr<etcdserverpb::PutResponse> reply{new etcdserverpb::PutResponse};
    grpc::ClientContext ctx;
    std::shared_ptr<grpc::Status> status{new grpc::Status};
    std::unique_ptr<grpc::ClientAsyncResponseReader<etcdserverpb::PutResponse>> reader;
    ResponseCallback callback;
    std::promise<ResponsePtr> promise;
};

class AsyncRangeCall final : public Callbase
{
public:
    AsyncRangeCall(std::string const &token, std::string const &key, std::string const &range_end)
    {
        ctx.AddMetadata("token", token);
        request.set_key(key);
        request.set_range_end(range_end);
    }

    void OnResponse() override
    {
        if (callback)
        {
            callback(std::make_shared<RangeResponse>(status, reply));
        }
        else
        {
            promise.set_value(std::make_shared<RangeResponse>(status, reply));
        }
    }

    bool IsDone() const override
    {
        return true;
    }

    etcdserverpb::RangeRequest request;
    std::shared_ptr<etcdserverpb::RangeResponse> reply{new etcdserverpb::RangeResponse};
    grpc::ClientContext ctx;
    std::shared_ptr<grpc::Status> status{new grpc::Status};
    std::unique_ptr<grpc::ClientAsyncResponseReader<etcdserverpb::RangeResponse>> reader;
    ResponseCallback callback;
    std::promise<ResponsePtr> promise;
};

enum WatchStatus
{
    initializing,
    writing,
    reading,
    connected,
    canceling,
    cancelled
};

class AsyncWatchCall final : public Callbase
{
public:
    AsyncWatchCall(
        std::string const &token, std::string const &k, std::string const &r, EventCallback &&cb)
        : key(k)
        , range_end(r)
        , watch_id(clk::now().time_since_epoch().count())
        , callback(std::move(cb))
    {
        ctx.AddMetadata("token", token);
    }

    ~AsyncWatchCall() override
    {
        spdlog::debug("AsyncWatchCall object {} destroyed", watch_id);
    }

    void CreateWatcher(etcdserverpb::Watch::Stub *stub, grpc::CompletionQueue *cqueue)
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (watcher_status == WatchStatus::initializing)
        {
            spdlog::debug("call AsyncWatch");
            stream = stub->AsyncWatch(&ctx, cqueue, this);
        }
    }

    void CancelWatcher()
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (watcher_status != WatchStatus::connected)
        {
            spdlog::error("cannot cancel a wacher which is not in a connected state");
            return;
        }
        watcher_status = WatchStatus::canceling;
        etcdserverpb::WatchRequest request;
        request.mutable_cancel_request()->set_watch_id(watch_id);
        grpc::WriteOptions opt;
        spdlog::debug("canceling watcher:{}, sending the last message", watch_id);
        stream->WriteLast(request, opt, this);
    }

    void OnResponse() override;

    bool IsDone() const override
    {
        return watcher_status == WatchStatus::cancelled;
    }

    const std::string key;
    const std::string range_end;
    const int64_t watch_id;
    WatchStatus watcher_status{WatchStatus::initializing};
    std::shared_ptr<etcdserverpb::WatchResponse> reply{new etcdserverpb::WatchResponse};
    grpc::ClientContext ctx;
    std::shared_ptr<grpc::Status> status{new grpc::Status};
    std::unique_ptr<
        grpc::ClientAsyncReaderWriter<etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>>
        stream;
    EventCallback callback;

private:
    std::mutex state_mutex;
};

} // namespace etcdv3