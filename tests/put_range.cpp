#include "etcdv3_client.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main(int argc, char const *argv[])
{
    auto logger = spdlog::stdout_color_mt("test_put_range");
    auto client = etcdv3::Client::Create(
        "http://172.253.172.144:2381,http://172.253.172.144:2383,http://172.253.172.144:2385",
        "test_user", "test_pass");

    auto put_resp = client->Put("Foo", "Bar");
    if (put_resp->Ok())
    {
        logger->info("PUT key success");
    }
    else
    {
        logger->info("PUT key error, message:", put_resp->ErrorMessage());
        return 1;
    }

    auto range_resp = client->Range("Foo", "");
    if (range_resp->Ok())
    {
        for (auto const &it = range_resp->KVS().begin(); it != range_resp->KVS().end(); ++it)
            logger->info("GET key success, value:", it->Key());
    }
    else
    {
        logger->info("GET key failed, message:", range_resp->ErrorMessage());
        return 1;
    }
    

    return 0;
}
