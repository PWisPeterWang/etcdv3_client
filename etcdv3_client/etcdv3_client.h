#pragma once
#include "etcdv3_response.h"
#include <future>
#include <memory>

namespace etcdv3 {

class Client
{
public:
    /**
     * @brief create an instance of etcdv3 Client
     * @param endpoints endpoints of etcdv3 cluster nodes, e.g.
     * 'http://172.253.172.144:2381,http://172.253.172.144:2383,http://172.253.172.144:2385'
     * @param user user name for authenticate
     * @param pass password for authentication
     * @return Client* the created instance of etcdv3 Client
     */
    static std::shared_ptr<Client> Create(
        std::string endpoints, std::string user, std::string pass);

    Client() = default;
    virtual ~Client() = default;

    /**
     * @brief Synchronous Put interface
     * @param key the key to put
     * @param value  the value of the key
     * @return PutResponse the response for this Put operation, containing the previous key/value if
     * any
     */
    virtual ResponsePtr Put(std::string const &key, std::string const &value) = 0;

    /**
     * @brief Asyncrhonous Put interface
     * @param key
     * @param value
     * @return std::future<PutResponse>
     */
    virtual std::future<ResponsePtr> AsyncPut(std::string const &key, std::string const &value) = 0;

    /**
     * @brief Asynchronous Put interface with callback
     *
     * @param key
     * @param value
     * @param callback
     */
    virtual void CallbackPut(
        std::string const &key, std::string const &value, ResponseCallback &&callback) = 0;

    /**
     * @brief Synchronous Range interface
     * @param key key to search
     * @param range_end key prefix end
     * @return RangeResponse the response containing the results
     */
    virtual ResponsePtr Range(std::string const &key, std::string const &range_end) = 0;

    /**
     * @brief Asynchronous Range interface with future style
     * @param key
     * @param range_end
     * @return RangeResponse
     */
    virtual std::future<ResponsePtr> AsyncRange(
        std::string const &key, std::string const &range_end) = 0;

    /**
     * @brief Asynchronous Range interface with callback style
     *
     * @param key
     * @param callback
     * @param range_end
     */
    virtual void CallbackRange(
        std::string const &key, ResponseCallback &&callback, std::string const &range_end) = 0;

    /**
     * @brief Create a Watch object
     * @param key key to watch
     * @param range_end  key range end
     * @return int64_t watch id
     */
    virtual int64_t CreateWatch(
        std::string key, std::string range_end, EventCallback &&callback) = 0;

    /**
     * @brief Set the callback for watcher with given id
     * @param id the watcher id
     * @return int64_t  > 0 watcher id, <0 if error
     */
    virtual int64_t CancelWatcher(int64_t id) = 0;
};

} // namespace etcdv3