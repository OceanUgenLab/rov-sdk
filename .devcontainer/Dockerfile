# ou_sdk 开发容器：g++ 12 + cmake + ninja + python3 + PyYAML + Jinja2
FROM ubuntu:22.04

# 避免安装过程中的交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 基础工具链 + g++-12（协议 codegen / golden_gen 需要 python3 + PyYAML + Jinja2）
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        g++-12 \
        gcc-12 \
        cmake \
        ninja-build \
        git \
        ca-certificates \
        python3 \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

# 将 g++-12 / gcc-12 设为默认编译器
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-12 100

# codegen / golden_gen 的 Python 依赖
RUN pip3 install --no-cache-dir pyyaml jinja2

# 默认工作目录（与 devcontainer.json 的 remoteUser 配合）
RUN useradd -m -s /bin/bash vscode || true
USER vscode
WORKDIR /workspaces/ou_sdk
