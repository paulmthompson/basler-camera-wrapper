FROM ubuntu:18.04

NV DEBIAN_FRONTEND noninteractive

# basic stuff
RUN echo 'APT::Get::Assume-Yes "true";' >> /etc/apt/apt.conf \
    && apt-get update && apt-get install \
    dpkg \
# Cleanup
    && rm -rf /tmp/* /var/lib/apt/lists/* /root/.cache/*

COPY pylon pylon
