#include "etcdv3_threadpool.h"
#include <chrono>
#include <grpc/impl/codegen/gpr_types.h>
#include <grpc/support/time.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/time.h>
#include <spdlog/spdlog.h>

namespace etcdv3 {

void Worker::ThreadFunc()
{
    spdlog::debug("worker {} start", m_idx);
    void *tag{};
    bool ruok{};
    while (!m_is_stopped)
    {
        gpr_timespec deadline =
            gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_millis(100, GPR_TIMESPAN));
        auto status = m_ptr_cqueue->AsyncNext(&tag, &ruok, deadline);
        if (status == grpc::CompletionQueue::GOT_EVENT)
        {
            auto *call = reinterpret_cast<Callbase *>(tag);
            if (!ruok)
            {
                spdlog::debug("worker {} call {} not ok", m_idx, call->Id());
                m_pool->RemoveCall(call->Id());
            }
            else
            {
                spdlog::debug("worker {} entering OnResponse", m_idx);
                call->OnResponse();
                spdlog::debug("worker {} leaving OnResponse", m_idx);
                if (call->IsDone())
                {
                    spdlog::debug("call is done, removing call id:{}", call->Id());
                    m_pool->RemoveCall(call->Id());
                }
            }
        }
        else if (status == grpc::CompletionQueue::SHUTDOWN)
        {
            m_is_stopped.store(true);
            break;
        }
        else if (status == grpc::CompletionQueue::TIMEOUT)
        {
            continue;
        }
    }
    m_is_stopped.store(true);
}
} // namespace etcdv3