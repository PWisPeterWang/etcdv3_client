#pragma once
#include "etcdv3_mvcckv.h"
#include <etcd/api/kv.pb.h>
#include <etcd/api/rpc.pb.h>
#include <google/protobuf/repeated_field.h>
#include <memory>

namespace etcdv3 {

extern const mvccpb::KeyValue null_kv;
extern const mvccpb::Event null_event;

class KVImpl final : public KV
{
public:
    KVImpl()
        : m_kv(&null_kv)
    {}
    explicit KVImpl(mvccpb::KeyValue const &kv)
        : m_kv(&kv)
    {}

    KVImpl(KVImpl const &other)
        : m_kv(other.m_kv)
    {}

    std::string const &Key() const override
    {
        return m_kv->key();
    }
    std::string const &Value() const override
    {
        return m_kv->value();
    }

private:
    mvccpb::KeyValue const *m_kv;
};

class KVIteratorImpl final : public KVIterator
{
public:
    KVIteratorImpl()
        : m_kv(null_kv)
    {}
    explicit KVIteratorImpl(google::protobuf::RepeatedPtrField<mvccpb::KeyValue>::const_iterator it)
        : m_it(it)
    {}

    KV const &operator*() const override
    {
        m_kv = KVImpl(*m_it);
        return m_kv;
    }

    KV const *operator->() const override
    {
        m_kv = KVImpl(*m_it);
        return &m_kv;
    }

    KVIterator const &operator++() const override
    {
        ++m_it;
        return *this;
    }

    bool operator!=(const KVIterator &other) const override
    {
        return m_it != static_cast<KVIteratorImpl const &>(other).m_it;
    }

private:
    mutable google::protobuf::RepeatedPtrField<mvccpb::KeyValue>::const_iterator m_it;
    mutable KVImpl m_kv;
};

class KVRangeImpl final : public KVRange
{
public:
    KVRangeImpl()
        : m_begin()
        , m_end()
    {}
    KVRangeImpl(KVIteratorImpl begin, KVIteratorImpl end)
        : m_begin(begin)
        , m_end(end)
    {}

    KVIterator const &begin() const override
    {
        return m_begin;
    }
    KVIterator const &end() const override
    {
        return m_end;
    }

private:
    KVIteratorImpl m_begin;
    KVIteratorImpl m_end;
};

class EventImpl final : public Event
{
public:
    EventImpl()
        : m_event(&null_event)
        , m_type(EventType::DELETE)
        , m_prev_kv(null_kv)
        , m_kv(null_kv)
    {}
    EventImpl(mvccpb::Event const &event)
        : m_event(&event)
        , m_type(static_cast<EventType>(event.type()))
        , m_prev_kv(m_event->prev_kv())
        , m_kv(m_event->kv())
    {}
    EventImpl(EventImpl const &other)
        : m_event(other.m_event)
        , m_type(other.m_type)
        , m_prev_kv(other.m_prev_kv)
        , m_kv(other.m_kv)
    {}

    KV const &PrevKV() const override
    {
        return m_prev_kv;
    }
    KV const &CurKV() const override
    {
        return m_kv;
    }
    EventType Type() const override
    {
        return m_type;
    }

private:
    mvccpb::Event const *m_event;
    EventType m_type;
    KVImpl m_prev_kv;
    KVImpl m_kv;
};

class EventIteratorImpl final : public EventIterator
{
public:
    EventIteratorImpl()
        : m_event(null_event)
    {}
    explicit EventIteratorImpl(
        google::protobuf::RepeatedPtrField<mvccpb::Event>::const_iterator const &it)

        : m_it(it)
    {}

    Event const &operator*() const override
    {
        m_event = EventImpl(*m_it);
        return m_event;
    }

    Event const *operator->() const override
    {
        m_event = EventImpl(*m_it);
        return &m_event;
    }

    EventIterator const &operator++() const override
    {
        ++m_it;
        return *this;
    }

    bool operator!=(const EventIterator &other) const override
    {
        return m_it != static_cast<EventIteratorImpl const &>(other).m_it;
    }

private:
    mutable google::protobuf::RepeatedPtrField<mvccpb::Event>::const_iterator m_it;
    mutable EventImpl m_event;
};

class EventRangeImpl final : public EventRange
{
public:
    EventRangeImpl() = default;
    EventRangeImpl(etcdserverpb::WatchResponse const &reply)
        : m_begin(reply.events().begin())
        , m_end(reply.events().end())
    {}

    EventIterator const &begin() const override
    {
        return m_begin;
    }
    EventIterator const &end() const override
    {
        return m_end;
    }

private:
    EventIteratorImpl m_begin;
    EventIteratorImpl m_end;
};

} // namespace etcdv3