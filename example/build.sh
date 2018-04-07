#!/bin/bash

g++ example.cpp -o example.o -I../include/ -std=c++11 -pthread && ./example.o