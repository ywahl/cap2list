cmake_minimum_required(VERSION 3.8)
project(cap2list)

set(CMAKE_CXX_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCE_FILES
        base.h
        config.cpp
        config.h
        main.cpp
        pcapintf.cpp
        pcapintf.h
        poll.cpp
        poll.h
        queue.cpp
        queue.h
        sockets.cpp
        sockets.h
        system.h
        test.cpp
        test.h
        timer.cpp
        timer.h kafkaproducer.cpp kafkaproducer.h)

add_executable(cap2list ${SOURCE_FILES})
target_link_libraries(cap2list pcap Threads::Threads rdkafka)