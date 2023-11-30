#include "etcdv3_client.h"
#include <cassert>

int main(int argc, char const *argv[])
{
    auto client = etcdv3::Client::Create(
        "http://172.253.172.144:2381,http://172.253.172.144:2383,http://172.253.172.144:2385",
        "test_user", "test_pass");

    assert(client != nullptr);
    getchar();
    return 0;
}
