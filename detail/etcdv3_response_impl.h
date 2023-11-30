#pragma once
#include "etcdv3_mvcckv.h"
#include "etcdv3_response.h"
#include "etcdv3_mvcckv_impl.h"
#include <etcd/api/rpc.pb.h>
#include <grpcpp/impl/codegen/status.h>
#include <memory>
#include <vector>

namespace etcdv3 {

extern KVRangeImpl null_kvs;

class PutResponse final : public Response
{
public:
    PutResponse(std::shared_ptr<grpc::Status> const &status,
        std::shared_ptr<etcdserverpb::PutResponse> const &reply)
        : m_status(status)
        , m_reply(reply)
        , m_prev_kv(reply->prev_kv())
    {}

    bool Ok() const override
    {
        return m_status->ok();
    }
    std::string ErrorMessage() const override
    {
        return m_status->error_message();
    }
    int64_t ErrorCode() const override
    {
        return m_status->error_code();
    }
    bool HasPrevKV() const override
    {
        return true;
    }
    KV const &PrevKV() const override
    {
        return m_prev_kv;
    }
    bool HasKVS() const override
    {
        return false;
    }
    KVRange const &KVS() const override
    {
        return null_kvs;
    }

private:
    std::shared_ptr<grpc::Status> m_status;
    std::shared_ptr<etcdserverpb::PutResponse> m_reply;
    KVImpl m_prev_kv;
};

class RangeResponse final : public Response
{
public:
    RangeResponse(std::shared_ptr<grpc::Status> const &status,
        std::shared_ptr<etcdserverpb::RangeResponse> const &reply)
        : m_status(status)
        , m_reply(reply)
        , m_kvs(KVIteratorImpl(reply->kvs().begin()), KVIteratorImpl(reply->kvs().end()))
    {}

    bool Ok() const override
    {
        return m_status->ok();
    }
    std::string ErrorMessage() const override
    {
        return m_status->error_message();
    }
    int64_t ErrorCode() const override
    {
        return m_status->error_code();
    }
    bool HasPrevKV() const override
    {
        return true;
    }
    KV const &PrevKV() const override
    {
        return m_prev_kv;
    }
    bool HasKVS() const override
    {
        return true;
    }
    KVRange const &KVS() const override
    {
        return m_kvs;
    }

private:
    std::shared_ptr<grpc::Status> m_status;
    std::shared_ptr<etcdserverpb::RangeResponse> m_reply;
    KVImpl m_prev_kv;
    KVRangeImpl m_kvs;
};

} // namespace etcdv3