#pragma once
#include "etcdv3_call.h"
#include <grpcpp/grpcpp.h>
#include <etcd/api/rpc.pb.h>
#include <memory>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>
#include <map>

namespace etcdv3 {
class ThreadPool;
class Worker
{
public:
    Worker(grpc::CompletionQueue *cqueue, ThreadPool *pool, uint idx)
        : m_ptr_cqueue(cqueue)
        , m_pool(pool)
        , m_idx(idx)
        , m_is_stopped(false)
    {}

    Worker(Worker &&other) noexcept
        : m_ptr_cqueue(other.m_ptr_cqueue)
        , m_pool(other.m_pool)
        , m_idx(other.m_idx)
        , m_is_stopped(other.m_is_stopped.load())
    {}

    ~Worker()
    {
        if (m_thread_ptr)
            m_thread_ptr->join();
    }

    void Start()
    {
        m_thread_ptr = std::unique_ptr<std::thread>(new std::thread([this] { ThreadFunc(); }));
    }

    void Stop()
    {
        m_is_stopped.store(true);
    }

private:
    void ThreadFunc();

    grpc::CompletionQueue *m_ptr_cqueue;
    ThreadPool *m_pool;
    const uint m_idx;
    std::atomic_bool m_is_stopped;
    std::unique_ptr<std::thread> m_thread_ptr;
};

class ThreadPool
{
    friend class Worker;

public:
    explicit ThreadPool(uint thread_num, grpc::CompletionQueue *cqueue)
        : m_thread_num(thread_num)
        , m_ptr_cqueue(cqueue)
    {
        spdlog::debug("thread pool initialized with {} workers", thread_num);
        m_workers.reserve(thread_num);
    }

    void Start()
    {
        for (uint idx = 0; idx < m_thread_num; ++idx)
        {
            spdlog::debug("starting worker: {}", idx);
            m_workers.emplace_back(m_ptr_cqueue, this, idx);
            m_workers.back().Start();
        }
        spdlog::debug("all workers started");
    }

    void Stop()
    {
        for (uint idx = 0; idx < m_thread_num; ++idx)
        {
            m_workers[idx].Stop();
        }
    }

    void AddCall(std::shared_ptr<Callbase> const &call)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_calls.emplace(call->Id(), call);
    }

    void RemoveCall(int64_t id)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_calls.erase(id);
    }

private:
    const uint m_thread_num;
    grpc::CompletionQueue *m_ptr_cqueue;
    std::mutex m_mtx;
    std::map<int64_t, std::shared_ptr<Callbase>> m_calls;
    std::vector<Worker> m_workers;
};

} // namespace etcdv3