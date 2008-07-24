#!/bin/sh

cat $@ | gzip -9 | tail -c +11 | head -c -8
