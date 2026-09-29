# Build and test environment for the NRS pass, the precision tests and the
# rocket-chip emulator. The sources are not part of the image: mount the
# directory holding llvm-project, rocket-chip and NRSSL-LLVMPass at
# /workspace/shared, see the README.
#
#   docker build -t nrs-dev --build-arg UID=$(id -u) --build-arg GID=$(id -g) docker
#   docker run -it --name nrs-dev -v <shared>:/workspace/shared nrs-dev

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    # build essentials
    git autoconf automake autotools-dev curl wget gnupg \
    build-essential cmake ninja-build pkg-config \
    # RISC-V toolchain and emulator dependencies
    libmpc-dev libmpfr-dev libgmp-dev libusb-1.0-0-dev \
    gawk bison flex texinfo gperf libtool patchutils bc \
    zlib1g-dev device-tree-compiler libexpat-dev libfl-dev \
    # Java for NRSSL (JNI), Scala and Mill
    openjdk-17-jdk scala \
    verilator \
    # LLVM/Clang for building the pass
    clang-19 llvm-19 lld-19 llvm-19-dev libclang-19-dev \
    python3 \
    vim less tree \
    && rm -rf /var/lib/apt/lists/*

# sbt, builds NRSSL
RUN curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" \
      | gpg --dearmor -o /usr/share/keyrings/sbt.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/sbt.gpg] https://repo.scala-sbt.org/scalasbt/debian all main" \
      > /etc/apt/sources.list.d/sbt.list \
    && apt-get update && apt-get install -y sbt \
    && rm -rf /var/lib/apt/lists/*

# Mill 0.11.7 for rocket-chip
RUN curl -L https://github.com/com-lihaoyi/mill/releases/download/0.11.7/0.11.7 \
      -o /usr/local/bin/mill && chmod +x /usr/local/bin/mill

# firtool (FIRRTL to Verilog)
RUN wget -q -O - https://github.com/llvm/circt/releases/download/firtool-1.56.1/circt-full-shared-linux-x64.tar.gz \
      | tar -zx -C /opt

# RISC-V GNU toolchain, pk and spike
RUN wget -q -O - https://github.com/chipsalliance/rocket-tools/releases/download/2023.01.30/rocket-tools-ubuntu-22.04-nightly-2023.01.30-nightly.tar.gz \
      | tar -zx && mv /riscv /opt/riscv

ENV PATH="/opt/firtool-1.56.1/bin:/opt/riscv/bin:/usr/lib/llvm-19/bin:${PATH}"
ENV RISCV="/opt/riscv"
ENV JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
ENV CPATH="${JAVA_HOME}/include:${JAVA_HOME}/include/linux"

# Work as a user with the host's UID/GID, so that build output in the mounted
# directory is not owned by root. The image's default "ubuntu" user would
# clash with UID 1000.
ARG UID=1000
ARG GID=1000
RUN userdel -r ubuntu 2>/dev/null; \
    groupadd -g ${GID} dev && useradd -m -u ${UID} -g ${GID} -s /bin/bash dev
USER dev

WORKDIR /workspace/shared
CMD ["/bin/bash"]
