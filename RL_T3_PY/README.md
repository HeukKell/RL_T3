# RL_T3 Python 학습 프로세스

Unreal Engine 환경과 Windows 공유 메모리로 통신하며, 5개 상태 × 3개 행동의 Q-table을 학습합니다.

전체 환경 구성, 상태·보상 정의, 실행 순서와 결과 해석은 [프로젝트 README](../README.md)를 참고하세요.

## 실행

이 폴더에서 Python 환경을 설치합니다.

```powershell
uv sync
```

Unreal에서 `BeginTrain()`을 호출하여 `Waiting for brain.` 로그가 출력된 뒤 실행합니다.

```powershell
uv run main.py
```

Python 3.11과 Windows가 필요합니다. 정상 학습 종료 시 현재 작업 디렉터리의 `data/model.txt`에 Q-table이 저장됩니다. 매 실행은 0으로 초기화한 Q-table로 시작하며, 기존 모델을 불러오는 기능은 없습니다.
