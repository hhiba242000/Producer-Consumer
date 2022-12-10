cmake_minimum_required(VERSION 3.24)
project(ProducerConsumer)

set(CMAKE_CXX_STANDARD 14)

add_executable(Consumer consumer.cpp)
add_executable(Producer
        producer.cpp )
