#pragma once
#include "etcdv3_mvcckv.h"
#include <functional>
#include <memory>
namespace etcdv3 {

class Response
{
public:
    Response() = default;
    virtual ~Response() = default;

    virtual bool Ok() const = 0;
    virtual std::string ErrorMessage() const = 0;
    virtual int64_t ErrorCode() const = 0;
    virtual bool HasPrevKV() const = 0;
    virtual KV const &PrevKV() const = 0;
    virtual bool HasKVS() const = 0;
    virtual KVRange const &KVS() const = 0;
};
using ResponsePtr = std::shared_ptr<Response>;
using ResponseCallback = std::function<void(ResponsePtr const &)>;
using EventCallback = std::function<void(EventRange &)>;
struct DefaultEventCallback
{
    void operator()(EventRange &events)
    {
        for (auto const &it = events.begin(); it != events.end(); ++it)
        {
            printf("event type: %s, key:%s, value:%s; prev_key:%s, prev_value:%s\n",
                (it->Type() == EventType::PUT ? "PUT" : "DELETE"), it->CurKV().Key().c_str(),
                it->CurKV().Value().c_str(), it->PrevKV().Key().c_str(),
                it->PrevKV().Value().c_str());
        }
    }
};
} // namespace etcdv3
