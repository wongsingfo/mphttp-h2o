# CompNet.Lab3: Multipath HTTP/2

Author: 

## Introduction

In the third lab, you are required to develop a HTTP/2-based downloader, which fetches HTTP objects over multiple network paths. The idea behind this lab originates from the paper [_MP-H2: A Client-only Multipath Solution for HTTP/2_]() (Nikravesh et al.). Before starting working on the lab, you are supposed to carefully read the paper for better understanding of how the downloader functions. 

For simplicity, we won't implement the downloader on Android devices, as described in the paper. Instead, the downloader will be implemented on the top of a HTTP/2 library in C programming language. Also, to make the things easier, you downloader will be run in an emulated environment with exactly two CDNs and one client. In order to emulate a network environment with two CDN servers, we will take advantage of [Linux namespaces](http://man7.org/linux/man-pages/man7/namespaces.7.html) to set up a virtual network on a Linux machine. We omit the DNS lookup here and the IP addresses of the servers are given. Two CDN servers serve the exactly same content. Your target in this lab is to reduce the overall downloading time for a given set of file download requests. We will measure the downloading time of your implementation under varied network conditions.

After finishing this lab, you are expected to:

- Get a taste of how the multpath solution boosts downloading performance.
- Be familiar with new features in HTTP/2, including stream multiplexing, flow control, and application-layer PING.
- Be capable of adding new features to an existing production-level library.

## Handout Instructions

This section will instruct you to set up the development environment on a Linux machine. We will evaluate your programs in the same environment.

### Download the HTTP/2 Library

Our lab is based on a mature HTTP/2 Library, named [H2O](https://github.com/h2o/h2o). After building the library on you Linux machine, you can:

- Follow the [Wiki](https://github.com/h2o/h2o/wiki) to run static content server.
- Develop your multipath downloader with the help of the library.

### Create a Virtual Network

Again, the [vnetUtils]() is our good friend. You may find its usage in handout of lab2. In lab3, we create a virtual network by running `makeVNet` with the following configuration (note that the last two blank lines is left intentionally):

```
H2
2
0 1 10.100.1
0 2 10.100.2


```

By now, in addition to the default namespace, you have already set up two virtual network namespaces. This can be verified by running the command `ip netns list`. You should run a content server in each network namespace, and your downloader will be run in the default namespace.

## Test and Evaluation

The grading of the final hand-in will be based on the download time your downloader takes on the given traces. Each trace contains:

- Two bandwidth and RTT profiles used to specify the network condition (imposed by Linux `tc`), one for each CDN server. 
- A list of the files the program need to download.

We will give you a handful of testing traces, which may be useful for debugging. However, the final evaluation traces are kept secret.

## Hints on Implementation

TODO

## Handin Instructions

In this lab, you should submit a directory named `lab3` containing the following items in an archive named `lab3-[your name]-[your student ID].tar`:

- `src/`: Source code of your programs
- `Makefile` / `CMakeLists.txt`: Files that instruct build system to build your programs. 

## Contact the Staff


