# README 이미지 제작 정보

모든 PNG는 저장소 루트의 README.md에서 상대 경로로 참조합니다.

- 00-project-cover.png: 이전에 내장 image_gen으로 생성한 썸네일을 원본 그대로 복사했습니다. 표 안 숫자는 설명용 예시입니다.
- 04-training-environment.png: 내장 image_gen으로 생성한 학습 환경 콘셉트 일러스트입니다. 실제 실행 화면이 아닙니다.
- 나머지 PNG 9개: 코드와 README의 수치·순서를 정확히 표현하기 위해 Pillow로 직접 그린 원본 설명 도식입니다. 생성형 이미지의 수정이나 변환에는 Pillow를 사용하지 않았습니다.
- 09-qtable-results.png: 대화에서 공유한 Q-table 값을 소수 셋째 자리까지 표시했습니다. 성공률을 측정한 그림이 아닙니다.

## 도식 재생성

Windows의 맑은 고딕 및 Segoe UI 글꼴, Pillow가 필요합니다. 저장소 루트에서 실행합니다.

```powershell
.\RL_T3_PY\.venv\Scripts\python.exe -B Asset/2D/build_readme_figures.py
```

이 스크립트는 설명 도식 9개를 다시 생성합니다. 표지와 학습 환경 일러스트는 수정하지 않습니다.

## 표지 생성 프롬프트

```text
Use case: ads-marketing
Asset type: one landscape 16:9 project thumbnail / repository cover for RL_T3.
Create a polished technical game-development thumbnail for an Unreal Engine and Python tabular Q-learning experiment. A striking, uncluttered 3D scene: a metallic game mannequin agent on a straight horizontal test track, moving toward a glowing target marker. The track is one-dimensional, not a maze. Clear directional arrows illustrate left, stop, right. Integrate a small elegant floating Q-table panel with exactly five rows and three columns, strongest cells highlighted in column order 1,1,2,3,3 from top to bottom, conveying a learned policy. The table should be secondary to the character and destination.
Style: premium Unreal-style 3D render mixed with crisp editorial typography; dark graphite environment, restrained cool cyan illumination and warm amber target, sharp silhouette, cinematic depth, clean composition that reads at thumbnail size. Strong visual hierarchy, large title with ample breathing room, no clutter.
Exact headline text: "Q-LEARNING"
Exact smaller subtitle: "UNREAL ENGINE × PYTHON"
Small project identifier: "RL_T3"
No other text required, no fake logos, no watermark, no neural-network brain imagery, no extra characters. This is conceptual cover artwork, not a screenshot. Keep key text and subject inside safe margins. Opaque background.
```

## 학습 환경 생성 프롬프트

```text
Use case: stylized-concept
Asset type: one wide 16:9 section illustration for the README of RL_T3, an Unreal Engine and Python tabular Q-learning project.
Create a clean cinematic 3D editorial illustration in the same visual family as a premium technical game-development cover: dark graphite background, cyan edge light, warm amber goal marker, metallic game mannequin.
Scene: a single straight one-dimensional training track viewed from an elevated oblique angle, not a maze or branching road. One metallic mannequin agent stands on the track to the left of a glowing amber circular target zone. Minimal cyan chevrons along the track imply motion toward the target, and a small square stop symbol sits in the target zone. The key idea is approach the target then stop. Sophisticated soft lighting, restrained glow, readable uncluttered silhouettes, generous negative space, no unrelated scenery, no UI screenshots.
Exact small title: "APPROACH → STOP"
Exact small secondary label: "RL_T3 / TRAINING ENVIRONMENT"
Do not show numbers, matrices, formulas, logos or additional text. This is a conceptual illustration, not an actual game screenshot. Opaque background.
```
