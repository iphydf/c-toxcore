#!/bin/sh

/cpachecker/scripts/cpa.sh -config /cpachecker/config/default.properties analysis.i 2>&1 | grep "Cannot compute size of incomplete type"
