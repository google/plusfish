# Building plusfish

## Install dependencies

We use bazel to build plusfish and you probably need to install it first:

```shell
sudo apt-get install bazel
```

Additionally openssl is required but that is usually already installed on most
systems.

## Building

Run the following command from the source directory:

```shell
bazel build :all
```

After this you can find the binary at bazel-bin/plusfish_cli

## Running the tests

```shell
bazel test ...:all
```




