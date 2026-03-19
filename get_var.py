#!/usr/bin/env python3

A = 13
B = 30103
C = 103

a = A + B
b = A + C

v1 = (a % 28) + 1
v2 = (b % 2) + 1
v3 = (A % 4) + 1

print(v1,v2,v3)
