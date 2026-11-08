# run_*.sh에서 불러 씀: OpenBCI 동글(시리얼 포트) 접근 권한 확인
if ! id -nG | grep -qw dialout; then
    echo "OpenBCI 동글에 접근하려면 dialout 그룹이 필요합니다:"
    echo "  sudo usermod -aG dialout $USER   (실행 후 로그아웃했다가 다시 로그인)"
fi
if [ ! -d /dev/serial/by-id ]; then
    echo "USB 시리얼 장치가 보이지 않습니다. OpenBCI 동글을 연결하세요."
fi
