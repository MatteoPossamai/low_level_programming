#!/bin/bash

for i in $(seq 1 3);
do
    ./build/subscriber "$i" 2>&1 &
done

wait
