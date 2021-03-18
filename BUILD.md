# Building plusfish

## Installing dependencies

Install the following packages if you're using Debian. Otherwise use the
equivalent packages available for your distro.

NOTE You need cmake 3.11 which might not be available on all distro's in
which case you'll have to build that from source first.

On Debian:

```shell
sudo apt-get install protobuf-compiler libprotobuf-dev libprotobuf17 libcurl4 \
   libcurl4-openssl-dev libgoogle-glog-dev libre2-5 libre2-dev libgumbo1 \
   libgumbo-dev git
 ```

On Ubuntu 18:

```shell
sudo apt-get install protobuf-compiler libprotobuf-dev libcurl4 \
  libcurl4-openssl-dev libgoogle-glog-dev libre2-4 libre2-dev libgumbo1 \
  libgumbo-dev git
```

If you don't have build tools installed, use the following command as well to
install the relevant packages.
```shell
sudo apt-get install cmake build-essential
```

## Building

```shell
cmake && make
```

