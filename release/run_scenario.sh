#!/bin/bash
# 시나리오 모드: config/Scenario.json 순서대로 비행하며 EEG 기록
# EEG 서버 주소 (IP:포트). 원격 서버인 경우 서버 PC의 IP로 설정
SERVER="127.0.0.1:9800"

cd "$(dirname "$0")"
. ./check_serial.sh
exec ./DroneSim.sh -nosplash -recording -EegServer="$SERVER" "$@"
