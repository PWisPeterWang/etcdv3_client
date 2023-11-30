# etcdv3_client
The project is still under development, but most of the common features are available.
Put, Range, Watch are supported. Lease is not yet supported. Thus, creating keys with lease is not supported.

For reference of usage, please refer to the tests directory.

## Build
```bash
cmake -H. -Bbuild -G Ninja
ninja -C build
```
## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
