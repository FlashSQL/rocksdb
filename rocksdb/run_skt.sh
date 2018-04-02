#!/bin/bash

./db_bench -benchmarks=sktseq -batch_size=1024  -key_size=25 -num=24000000 \
			-value_size=1024 -bloom_bits=32 -statistics=1 \
			-stats_dump_period_sec=5

			#-stats_per_interval=1 -stats_interval=1000000 
