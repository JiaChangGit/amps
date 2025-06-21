FROM ubuntu:22.04

## HOW to use
# sudo docker build -t multiple_parallel_pc .
# docker run --rm -it multiple_parallel_pc
# docker run --rm -it \
#     -v $(pwd)/../classbench_set:/workspace/classbench_set \
#     multiple_parallel_pc



# ====================
# Basic Environment
# ====================
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        curl \
        libeigen3-dev \
        libomp-dev \
        python3.10 \
        python3.10-dev \
        python3-pip \
        pybind11-dev \
        && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# ====================
# Python & Dependencies
# ====================
RUN ln -sf /usr/bin/python3.10 /usr/bin/python && \
    ln -sf /usr/bin/pip3 /usr/bin/pip && \
    pip install --upgrade pip

RUN pip install \
    --retries=10 \
    --timeout=60 \
    --progress-bar off \
    pybind11 \
    gymnasium \
    "ray[rllib]" \
    torch \
    numpy

# ====================
# Copy Project
# ====================
WORKDIR /workspace
COPY . /workspace

# ====================
# Build Project
# ====================
RUN chmod +x ./scripts/*.sh
RUN bash ./scripts/run.sh
# Or
# RUN bash ./scripts/per_run.sh

# ====================
# Default Entry
# ====================
CMD ["bash"]
