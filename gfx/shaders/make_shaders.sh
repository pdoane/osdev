#!/bin/sh
intel-gen4asm -g 7 -W -b passthrough_p.vs -o  passthrough_p.vs.c
intel-gen4asm -g 7 -W -b solid_ff8040ff.ps -l solid_ff8040ff.ps_entry -o solid_ff8040ff.ps.c -e solid_ff8040ff.ps.h


