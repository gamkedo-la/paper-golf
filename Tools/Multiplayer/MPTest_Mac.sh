#!/bin/bash


timestamp=$(date +'%Y-%m-%d_%H-%M-%S')
#logfile="${PWD}/../../Build/Logs/PaperGolfDevelopmentMac.${timestamp}.log"
logfile="../../Build/Logs/PaperGolfDevelopmentMac.${timestamp}.log"
echo "Writing log to $logfile"

../../Build/Development/Mac/PaperGolf.app/Contents/MacOS/PaperGolf -log >> $logfile 2>&1

# Doesn't work - no log file generated either with -log or -abslog (supposed to support absolute path file)
# ../../Build/Development/Mac/PaperGolf.app/Contents/MacOS/PaperGolf -ABSLOG="$logfile"
