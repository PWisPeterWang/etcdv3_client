#include "etcdv3_client_impl.h"
#include "etcdv3_client.h"
#include "etcdv3_call.h"
#include "etcdv3_response.h"
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <etcdv3_threadpool.h>
#include <chrono>
#include <etcd/api/rpc.grpc.pb.h>
#include <etcd/api/rpc.pb.h>

namespace etcdv3 {

ClientImpl::ClientImpl(std::shared_ptr<grpc::Channel> channel, std::string user, std::string pass)
    : m_channel(channel)
    // , m_logger(spdlog::rotating_logger_mt("etcdv3.Client", "etcdv3_client.log", 0x800000U, 10))
    , m_authenticator(channel, user, pass)
    , m_kv_stub(etcdserverpb::KV::NewStub(channel))
    , m_watch_stub(etcdserverpb::Watch::NewStub(channel))
    , m_lease_stub(etcdserverpb::Lease::NewStub(channel))
    , m_thread_pool(std::min(std::thread::hardware_concurrency() / 4, 4U), &m_cqueue)
{
    m_thread_pool.Start();
}

ClientImpl::~ClientImpl()
{
    m_authenticator.Stop();
    m_thread_pool.Stop();
    m_cqueue.Shutdown();
}

ResponsePtr ClientImpl::Put(std::string const &key, std::string const &value)
{
    return AsyncPut(key, value).get();
}

std::future<ResponsePtr> ClientImpl::AsyncPut(std::string const &key, std::string const &value)
{
    auto call = std::make_shared<AsyncPutCall>(m_authenticator.GetToken(), key, value);
    call->reader = m_kv_stub->AsyncPut(&call->ctx, call->request, &m_cqueue);
    call->reader->Finish(call->reply.get(), call->status.get(), call.get());
    m_thread_pool.AddCall(call);
    spdlog::debug("async put call, id:{}, key:{}, value:{} initiated", call->Id(), key, value);
    return call->promise.get_future();
}

void ClientImpl::CallbackPut(
    std::string const &key, std::string const &value, ResponseCallback &&callback)
{
    auto call = std::make_shared<AsyncPutCall>(m_authenticator.GetToken(), key, value);

    call->callback = std::move(callback);
    call->reader = m_kv_stub->AsyncPut(&call->ctx, call->request, &m_cqueue);
    call->reader->Finish(call->reply.get(), call->status.get(), call.get());
    m_thread_pool.AddCall(call);
}

ResponsePtr ClientImpl::Range(std::string const &key, std::string const &range_end)
{
    return AsyncRange(key, range_end).get();
}

std::future<ResponsePtr> ClientImpl::AsyncRange(
    std::string const &key, std::string const &range_end)
{
    auto call = std::make_shared<AsyncRangeCall>(m_authenticator.GetToken(), key, range_end);

    call->reader = m_kv_stub->AsyncRange(&call->ctx, call->request, &m_cqueue);
    call->reader->Finish(call->reply.get(), call->status.get(), call.get());
    m_thread_pool.AddCall(call);
    spdlog::debug("async range call, id:{}, key:{}, range_end:{}", call->Id(), key, range_end);
    return call->promise.get_future();
}

void ClientImpl::CallbackRange(
    std::string const &key, ResponseCallback &&callback, std::string const &range_end)

{
    auto call = std::make_shared<AsyncRangeCall>(m_authenticator.GetToken(), key, range_end);
    call->callback = std::move(callback);
    call->reader = m_kv_stub->AsyncRange(&call->ctx, call->request, &m_cqueue);
    call->reader->Finish(call->reply.get(), call->status.get(), call.get());
    m_thread_pool.AddCall(call);
}

int64_t ClientImpl::CreateWatch(std::string key, std::string range_end, EventCallback &&callback)
{
    auto call = std::make_shared<AsyncWatchCall>(
        m_authenticator.GetToken(), key, range_end, std::move(callback));
    m_thread_pool.AddCall(call);

    {
        std::lock_guard<std::mutex> lock(m_map_mtx);
        m_watchers.emplace(call->watch_id, call);
    }

    call->CreateWatcher(m_watch_stub.get(), &m_cqueue);
    return call->watch_id;
}

int64_t ClientImpl::CancelWatcher(int64_t id)
{
    std::unique_lock<std::mutex> lock(m_map_mtx);
    auto iter = m_watchers.find(id);
    if (iter == m_watchers.end())
    {
        return -1;
    }
    lock.unlock();

    iter->second->CancelWatcher();

    lock.lock();
    m_watchers.erase(iter->second->watch_id);
    lock.unlock();
    return id;
}

} // namespace etcdv3