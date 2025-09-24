#!/usr/bin/env bash
set -euo pipefail

# 適用：Ubuntu/Debian 系（需要 sudo 权限）
# 安裝：gRPC/Protobuf、MariaDB client、Kafka(librdkafka) 基础、Redis(hiredis)，
# 以及從源碼安裝 cppkafka、redis-plus-plus、muduo（可選）。

echo "[1/7] 更新套件索引..."
sudo apt-get update -y

echo "[2/7] 安裝編譯工具與核心依賴..."
sudo apt-get install -y \
  build-essential cmake pkg-config git curl \
  libprotobuf-dev protobuf-compiler protobuf-compiler-grpc libgrpc++-dev \
  libmariadb-dev \
  librdkafka-dev \
  libhiredis-dev \
  libssl-dev libcurl4-openssl-dev \
  nlohmann-json3-dev

echo "[3/7] 安裝 cppkafka（若已安裝可跳过）..."
if ! pkg-config --exists cppkafka 2>/dev/null; then
  TMP_DIR=$(mktemp -d)
  pushd "$TMP_DIR" >/dev/null
  git clone --depth 1 https://github.com/mfontanini/cppkafka.git
  cd cppkafka && mkdir build && cd build
  cmake .. -DCPPKAFKA_DISABLE_TESTS=ON
  make -j"$(nproc)"
  sudo make install
  sudo ldconfig
  popd >/dev/null
  rm -rf "$TMP_DIR"
else
  echo "cppkafka 已存在，跳过。"
fi

echo "[4/7] 安裝 redis-plus-plus（若已安裝可跳过）..."
if ! pkg-config --exists redis++ 2>/dev/null; then
  TMP_DIR=$(mktemp -d)
  pushd "$TMP_DIR" >/dev/null
  git clone --depth 1 https://github.com/sewenew/redis-plus-plus.git
  cd redis-plus-plus && mkdir build && cd build
  cmake .. -DREDIS_PLUS_PLUS_BUILD_SHARED=ON
  make -j"$(nproc)"
  sudo make install
  sudo ldconfig
  popd >/dev/null
  rm -rf "$TMP_DIR"
else
  echo "redis-plus-plus 已存在，跳过。"
fi

echo "[5/7] 安裝 muduo（可選，若已安裝可跳过）..."
if [ ! -f "/usr/local/lib/libmuduo_net.so" ] && [ ! -f "/usr/lib/libmuduo_net.so" ]; then
  TMP_DIR=$(mktemp -d)
  pushd "$TMP_DIR" >/dev/null
  git clone --depth 1 https://github.com/chenshuo/muduo.git
  cd muduo && mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j"$(nproc)"
  sudo make install
  sudo ldconfig
  popd >/dev/null
  rm -rf "$TMP_DIR"
else
  echo "muduo 已存在，跳过。"
fi

echo "[6/7] 安裝 spdlog（結构化日誌庫）..."
if ! pkg-config --exists spdlog 2>/dev/null; then
  TMP_DIR=$(mktemp -d)
  pushd "$TMP_DIR" >/dev/null
  git clone --depth 1 https://github.com/gabime/spdlog.git
  cd spdlog && mkdir build && cd build
  cmake .. -DSPDLOG_BUILD_EXAMPLE=OFF
  make -j"$(nproc)"
  sudo make install
  sudo ldconfig
  popd >/dev/null
  rm -rf "$TMP_DIR"
else
  echo "spdlog 已存在，跳过。"
fi

echo "[7/7] 完成。建議执行：sudo ldconfig"
echo "完成後可在專案根执行：mkdir -p build && cd build && cmake -DBUILD_MICROSERVICES=ON .. && make -j$(nproc)"




