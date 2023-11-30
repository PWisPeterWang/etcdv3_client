#pragma once
#include "etcdv3_client.h"
#include "etcdv3_call.h"
#include "etcdv3_threadpool.h"
#include "etcdv3_authenticator.h"
#include <cstdint>
#include <future>
#include <grpcpp/grpcpp.h>
#include <etcd/api/rpc.grpc.pb.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace etcdv3 {

class ClientImpl final : public Client
{
public:
    explicit ClientImpl(std::shared_ptr<grpc::Channel> channel, std::string user, std::string pass);
    ~ClientImpl() override;
    ResponsePtr Put(std::string const &key, std::string const &value) override;
    std::future<ResponsePtr> AsyncPut(std::string const &key, std::string const &value) override;
    void CallbackPut(
        std::string const &key, std::string const &value, ResponseCallback &&callback) override;
    ResponsePtr Range(std::string const &key, std::string const &range_end) override;
    std::future<ResponsePtr> AsyncRange(
        std::string const &key, std::string const &range_end) override;
    void CallbackRange(
        std::string const &key, ResponseCallback &&callback, std::string const &range_end) override;
    int64_t CreateWatch(std::string key, std::string range_end, EventCallback &&callback) override;
    int64_t CancelWatcher(int64_t watch_id) override;

private:
    std::shared_ptr<grpc::Channel> m_channel;
    // std::shared_ptr<spdlog::logger> m_logger;
    std::unique_ptr<etcdserverpb::KV::Stub> m_kv_stub;
    std::unique_ptr<etcdserverpb::Watch::Stub> m_watch_stub;
    std::unique_ptr<etcdserverpb::Lease::Stub> m_lease_stub;
    grpc::CompletionQueue m_cqueue;
    Authenticator m_authenticator;
    ThreadPool m_thread_pool;

    std::mutex m_map_mtx;
    std::map<int64_t, std::shared_ptr<AsyncWatchCall>> m_watchers;
};

} // namespace etcdv3