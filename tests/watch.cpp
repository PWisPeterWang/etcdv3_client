#include "etcdv3_client.h"
#include "etcdv3_mvcckv.h"
#include "etcdv3_response.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main(int argc, char const *argv[])
{
    auto logger = spdlog::stdout_color_mt("test_watch");
    auto client = etcdv3::Client::Create(
        "http://172.253.172.144:2381,http://172.253.172.144:2383,http://172.253.172.144:2385",
        "test_user", "test_pass");

    int cnt = 0;
    int64_t watcher_id = 0;
    watcher_id = client->CreateWatch("Foo", "",
        [&cnt, logger, client, &watcher_id](etcdv3::EventRange const &events) {
            for (auto const &iter = events.begin(); iter != events.end(); ++iter)
            {
                logger->info("EventType: {}, key:{}, value:{}, prev_key:{}, prev_value:{}",
                    (iter->Type() == etcdv3::EventType::PUT ? "PUT" : "DELETE"), iter->CurKV().Key(),
                    iter->CurKV().Value(), iter->PrevKV().Key(), iter->PrevKV().Value());
            }
        });
    logger->info("watcher id {} created", watcher_id);

    getchar();
    logger->info("canceling watcher_id:{}", watcher_id);
    client->CancelWatcher(watcher_id);
    getchar();
    return 0;
}
