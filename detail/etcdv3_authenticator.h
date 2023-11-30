#pragma once
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <etcd/api/rpc.grpc.pb.h>
#include <future>
#include <spdlog/spdlog.h>

namespace etcdv3 {
using clk = std::chrono::system_clock;
using boost::asio::io_context;
using boost::asio::steady_timer;

class Authenticator
{
public:
    Authenticator(std::shared_ptr<grpc::Channel> channel, std::string user, std::string pass,
        int64_t ttl = 300)
        : m_stub(etcdserverpb::Auth::NewStub(channel))
        , m_user(user)
        , m_pass(pass)
        , m_ttl(ttl)
        , m_timer(m_ioc)
        , m_stop(false)
        , m_refresh_thread([this] { TokenRefreshThread(); })
    {
        ReloadToken();
    }

    ~Authenticator()
    {
        m_refresh_thread.join();
    }

    std::string const &GetToken()
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_token;
    }

    void Stop()
    {
        m_stop.store(true);
        boost::system::error_code ignored;
        m_timer.cancel(ignored);
    }

private:
    void TokenRefreshThread()
    {
        ResetTimer();
        m_ioc.run();
    }

    void ResetTimer()
    {
        m_timer.expires_after(std::chrono::seconds(std::max(m_ttl - 3, 1L)));
        m_timer.async_wait([this](boost::system::error_code err) {
            if (!err)
            {
                ReloadToken();
                ResetTimer();
            }
            else if (err == boost::asio::error::operation_aborted)
            {
                spdlog::debug("timer canceled, exit thread");
                return;
            }
            else
            {
                assert(false);
            }
        });
    }

    void ReloadToken()
    {
        etcdserverpb::AuthenticateRequest req;
        etcdserverpb::AuthenticateResponse resp;
        grpc::ClientContext ctx;
        req.set_name(m_user);
        req.set_password(m_pass);

        auto status = m_stub->Authenticate(&ctx, req, &resp);
        if (!status.ok())
        {
            spdlog::error("authenticate error, msg:{}", status.error_message());
            assert(false);
            return;
        }
        spdlog::debug("auth token: {}", resp.token());
        std::lock_guard<std::mutex> lock(m_mtx);
        m_token = resp.token();
    }

    std::unique_ptr<etcdserverpb::Auth::Stub> m_stub;
    const std::string m_user;
    const std::string m_pass;
    const int64_t m_ttl;
    io_context m_ioc;
    steady_timer m_timer;
    std::atomic_bool m_stop;
    std::thread m_refresh_thread;
    std::mutex m_mtx;
    std::string m_token;
};
} // namespace etcdv3