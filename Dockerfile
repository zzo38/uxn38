# Debian oldstable (Bullseye)
FROM debian:oldstable

ARG DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-lc"]

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    build-essential \
    libsdl2-dev \
    rsync \
    wget \
    zip \
    git \
    ca-certificates \
    bash \
 && rm -rf /var/lib/apt/lists/*

# Headless environment (no real sound or display)
ENV XDG_RUNTIME_DIR=/tmp/runtime \
    SDL_AUDIODRIVER=dummy \
    SDL_VIDEODRIVER=dummy

RUN mkdir -p "$XDG_RUNTIME_DIR" && chmod 700 "$XDG_RUNTIME_DIR"

# Everything goes in /app
WORKDIR /app
COPY . .

# Build uxn (Linux) directly in /app
RUN set -eux; \
    gcc uxnasm.c -o uxnasm


