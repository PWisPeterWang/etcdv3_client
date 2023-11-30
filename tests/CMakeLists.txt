add_executable(test_connection connection.cpp)
target_link_libraries(test_connection
    PRIVATE
        etcdv3_client)

add_executable(test_put_range put_range.cpp)
target_link_libraries(test_put_range
    PRIVATE
        spdlog::spdlog
        etcdv3_client)

add_executable(test_watch watch.cpp)
target_link_libraries(test_watch
    PRIVATE
        etcdv3_client)