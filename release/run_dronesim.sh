#!/bin/bash
# 실행 모드: AI 모델 추론으로 드론 조작
# EEG 서버 주소 (IP:포트). 원격 서버인 경우 서버 PC의 IP로 설정
SERVER="127.0.0.1:9800"

cd "$(dirname "$0")"
. ./check_serial.sh
exec ./DroneSim.sh -nosplash -EegServer="$SERVER" "$@"
