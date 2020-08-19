FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN echo 'APT::Get::Assume-Yes "true";' >> /etc/apt/apt.conf \
    && apt-get update && apt-get install \
    dpkg \
# Cleanup
    && rm -rf /tmp/* /var/lib/apt/lists/* /root/.cache/*

COPY pylon pylon
COPY Build Build
COPY src src

RUN dpkg -i "$(ls pylon/*.deb)"

ARG PYLON_VERSION=6.1

RUN echo 'APT::Get::Assume-Yes "true";' >> /etc/apt/apt.conf \
    && apt-get update && apt-get install \
    build-essential \
    && bash Build/Linux/compile.sh $PYLON_VERSION \
# Cleanup
    && apt-get purge build-essential \
    && apt-get autoremove \
    && rm -rf /tmp/* /var/lib/apt/lists/* /root/.cache/*
