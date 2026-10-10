# ==============================================================================
# Multi-stage Dockerfile for BasilMart C++ Drogon Backend
# Builds on Debian/Ubuntu Linux and produces a lightweight, secure production image
# ==============================================================================

# Stage 1: Build environment
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install core build toolchain and Drogon + libpq dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    ninja-build \
    ca-certificates \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libssl-dev \
    libpq-dev \
    libc-ares-dev \
    libbrotli-dev \
    && rm -rf /var/lib/apt/lists/*

# Build and install Drogon framework from source (pinned to stable release v1.9.9)
WORKDIR /drogon-src
RUN git clone --depth 1 --branch v1.9.9 --recurse-submodules https://github.com/drogonframework/drogon.git . \
    && mkdir build \
    && cd build \
    && cmake -G Ninja \
             -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_CXX_FLAGS="-O2 -pipe --param ggc-min-expand=20 --param ggc-min-heapsize=32768" \
             -DCMAKE_INSTALL_LIBDIR=lib \
             -DBUILD_CTL=OFF \
             -DBUILD_EXAMPLES=OFF \
             -DBUILD_TESTING=OFF \
             -DBUILD_DOC=OFF \
             -DBUILD_POSTGRESQL=ON \
             -DBUILD_MYSQL=OFF \
             -DBUILD_SQLITE=OFF \
             -DBUILD_REDIS=OFF .. \
    && ninja -j 1 \
    && ninja install \
    && cd / \
    && rm -rf /drogon-src

# Copy BasilMart application source files
WORKDIR /app
COPY CMakeLists.txt config.json ./
COPY src/ ./src/

# Compile BasilMart binary (using 1 parallel job to stay strictly within Render memory constraints)
RUN mkdir -p build \
    && cd build \
    && cmake -G Ninja \
             -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_CXX_FLAGS="-O2 -pipe --param ggc-min-expand=20 --param ggc-min-heapsize=32768" .. \
    && ninja -j 1

# Stage 2: Minimal Production Runtime
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV PORT=8080

# Install only shared runtime libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libjsoncpp25 \
    libuuid1 \
    zlib1g \
    libssl3 \
    libpq5 \
    libc-ares2 \
    libbrotli1 \
    && rm -rf /var/lib/apt/lists/*

# Configure dynamic library loader path
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/drogon.conf

# Create a dedicated non-root user for security
RUN useradd -m -u 10001 basilmart

WORKDIR /app

# Copy Drogon installed libraries from builder stage
COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig

# Copy compiled executable and base configuration
COPY --from=builder /app/build/BasilMart /app/BasilMart
COPY --from=builder /app/config.json /app/config.json

# Create uploads directory and set permissions
RUN mkdir -p /app/uploads && chown -R basilmart:basilmart /app

USER basilmart

EXPOSE 8080

CMD ["/app/BasilMart"]
