FROM ubuntu:20.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Update and install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsdl2-dev \
    libfreeimage-dev \
    libfreetype6-dev \
    libcurl4-openssl-dev \
    rapidjson-dev \
    libasound2-dev \
    libvlc-dev \
    libvlccore-dev \
    vlc-bin \
    libsdl2-mixer-dev \
    libboost-all-dev \
    libeigen3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libcec-dev \
    libudev-dev \
    gettext \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /src

# Default command
CMD ["bash"]
