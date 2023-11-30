#include "etcdv3_client_impl.h"
#include <boost/regex.hpp>
#ifndef NDEBUG
#include <spdlog/sinks/stdout_color_sinks.h>
#else
#include <spdlog/sinks/rotating_file_sinks.h>
#endif

namespace etcdv3 {

static Client* CreateOnce(std::string endpoints, std::string user, std::string pass)
{
#ifndef NDEBUG
    auto logger = spdlog::stdout_color_mt("etcdv3.client");
    logger->set_level(spdlog::level::debug);
#else
    auto logger = spdlog::rotating_file_sink_mt("etcdv3.client", 0x800000U, 10);
#endif
    spdlog::set_default_logger(logger);

    std::vector<std::string> urls;
    boost::regex pattern(R"=(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:\d{1,5})=");
    boost::sregex_token_iterator iter(endpoints.begin(), endpoints.end(), pattern, 0);
    boost::sregex_token_iterator end;
    while (iter != end)
    {
        urls.emplace_back(*iter++);
    }
    auto url = fmt::format("ipv4:///{}", fmt::join(urls, ","));
    spdlog::info("creating gRPC channel with endpoints:{}", url);
    grpc::ChannelArguments grpc_args;
    grpc_args.SetMaxSendMessageSize(std::numeric_limits<int>::max());
    grpc_args.SetMaxReceiveMessageSize(std::numeric_limits<int>::max());
    grpc_args.SetLoadBalancingPolicyName("round_robin");
    std::shared_ptr<grpc::Channel> channel(
        grpc::CreateCustomChannel(url, grpc::InsecureChannelCredentials(), grpc_args));
    return new ClientImpl(channel, user, pass);
}

std::shared_ptr<Client> Client::Create(std::string endpoints, std::string user, std::string pass)
{
    static std::shared_ptr<Client> singleton(CreateOnce(endpoints, user, pass));
    return singleton;
}

} // namespace etcdv3