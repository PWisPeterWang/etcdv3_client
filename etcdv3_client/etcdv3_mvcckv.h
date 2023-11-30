#pragma once
#include <string>
#include <vector>
#include <memory>

namespace etcdv3 {

class KV
{
public:
    KV() = default;
    virtual ~KV() = default;

    virtual std::string const &Key() const = 0;
    virtual std::string const &Value() const = 0;
};

class KVIterator
{
public:
    virtual ~KVIterator() = default;
    virtual KV const &operator*() const = 0;
    virtual KV const *operator->() const = 0;
    virtual KVIterator const &operator++() const = 0;
    virtual bool operator!=(const KVIterator &other) const = 0;
};

class KVRange
{
public:
    virtual ~KVRange() = default;
    virtual KVIterator const &begin() const = 0;
    virtual KVIterator const &end() const = 0;
};

enum class EventType
{
    PUT = 0,
    DELETE = 1,
};

class Event
{
public:
    Event() = default;
    virtual ~Event() = default;
    virtual KV const &PrevKV() const = 0;
    virtual KV const &CurKV() const = 0;
    virtual EventType Type() const = 0;
};

class EventIterator
{
public:
    virtual ~EventIterator() = default;
    virtual Event const &operator*() const = 0;
    virtual Event const *operator->() const = 0;
    virtual EventIterator const &operator++() const = 0;
    virtual bool operator!=(const EventIterator &other) const = 0;
};

class EventRange
{
public:
    virtual ~EventRange() = default;
    virtual EventIterator const &begin() const = 0;
    virtual EventIterator const &end() const = 0;
};

} // namespace etcdv3