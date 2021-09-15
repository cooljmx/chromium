> :warning: This instruction was tested in Ubuntu 18.04 <br/>
> :warning: You must have Git and Python v3 installed already (and python3 must point to a Python v3 binary)<br/>
> :warning: It's required to checkout `spotware/sandbox` branch as it was create from stable (for spotware) commit

# Preparations

Move to home folder:
```shell
$ cd ~
```

Create workdir `spotware`:
```shell
$ mkdir ~/spotware && cd ~/spotware
```

Clone the `depot_tools` repository:
```shell
$ git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Add `depot_tools` to the end of your PATH:
```shell
$ export PATH="$PATH:${HOME}/spotware/depot_tools"
```

Create a `chromium` folder for the checkout and change to it:
```shell
$ mkdir ~/spotware/chromium && cd ~/spotware/chromium
```

Create `gclient` config:
```shell
$ gclient config --name=src https://github.com/cooljmx/chromium.git
```

Clone forked repository:
```shell
$ git clone --depth 1 -b spotware/sandbox https://github.com/cooljmx/chromium.git src
```

Run synchronization (put the last commit hash):
> :warning: it will take ~1 hour
```shell
$ gclient sync --no-history --revision 8df91161c37fa220b4f34522f6a461e2582e7466
```

Move to `src` folder:
```shell
$ cd src
```

Install additional build dependencies:
```shell
$ ./build/install-build-deps.sh
```

# Build

Generate ninja files:
```shell
$ gn gen --filters="//algo/linux:broker;//algo/linux:host" --ide=vs2019 out/algo
```

Build changes:
```shell
$ ninja -C out/algo algo/linux:broker algo/linux:host
```

You can find build result in `~/spotware/chromium/src/out/algo:`
- `algobroker.netcore`
- `algohost.netcore`