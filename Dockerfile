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

# Build and install Drogon framework from source
WORKDIR /drogon-src
RUN git clone --depth 1 --recurse-submodules https://github.com/drogonframework/drogon.git . \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release \
             -DBUILD_EXAMPLES=OFF \
             -DBUILD_TESTING=OFF \
             -DBUILD_POSTGRESQL=ON .. \
    && cmake --build . -j$(nproc) \
    && cmake --install . \
    && cd / \
    && rm -rf /drogon-src

# Copy BasilMart application source files
WORKDIR /app
COPY CMakeLists.txt config.json ./
COPY src/ ./src/

# Compile BasilMart binary
RUN mkdir -p build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && cmake --build . -j$(nproc)

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

# Create a dedicated non-root user for security
RUN useradd -m -u 10001 basilmart

WORKDIR /app

# Copy Drogon installed libraries from builder stage
COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig

# Copy compiled executable and base configuration
COPY --from=builder /app/build/BasilMart /app/BasilMart
COPY --from=builder /app/config.json /app/config.json

# Set ownership
RUN chown -R basilmart:basilmart /app

USER basilmart

EXPOSE 8080

CMD ["/app/BasilMart"]
