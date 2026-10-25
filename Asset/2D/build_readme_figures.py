"""Generate original, deterministic README diagrams (requires Pillow).

The generated cover and environment artwork are separate image_gen assets;
this script does not modify them. All paths are relative to this script.
"""
from pathlib import Path
from functools import lru_cache
import math

from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
W, H = 1800, 1000
BG = '#0b121c'
PANEL = '#121f2e'
LINE = '#294055'
WHITE = '#edf5fc'
MUTED = '#9cabbc'
CYAN = '#55dbef'
AMBER = '#ffca72'
RED = '#f59599'
FONT = Path('C:/Windows/Fonts/malgun.ttf')
BOLD = Path('C:/Windows/Fonts/malgunbd.ttf')


@lru_cache(None)
def font(size, bold=False, latin=False):
    if latin:
        path = Path('C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf')
    else:
        path = BOLD if bold else FONT
    return ImageFont.truetype(str(path), size)


def text_font(value, size, bold):
    return font(size, bold, not any('\uac00' <= c <= '\ud7a3' for c in str(value)))


class Figure:
    def __init__(self, section, title, subtitle):
        self.im = Image.new('RGB', (W, H), BG)
        self.d = ImageDraw.Draw(self.im)
        self.d.rectangle((0, 0, W, 9), fill=CYAN)
        self.text(78, 48, f'RL_T3  /  {section}', 22, CYAN, True)
        self.text(78, 102, title, 54, WHITE, True)
        self.text(78, 186, subtitle, 27, MUTED)
        self.d.line((78, 920, W-78, 920), fill=LINE, width=2)
        self.text(78, 944, 'UNREAL ENGINE × PYTHON', 18, MUTED)
        self.text(1420, 944, 'TABULAR Q-LEARNING', 18, MUTED)

    def text(self, x, y, value, size=30, color=WHITE, bold=False):
        value = str(value).replace('−', '-')
        for line in value.split('\n'):
            face = text_font(line, size, bold)
            bbox = self.d.textbbox((x, y), line, font=face)
            assert bbox[2] <= W-35 and bbox[3] <= H-15, (value, bbox)
            self.d.text((x, y), line, font=face, fill=color)
            y += round(size * 1.6)

    def center(self, x, y, value, size=30, color=WHITE, bold=False):
        value = str(value).replace('−', '-')
        width = self.d.textlength(value, font=text_font(value, size, bold))
        self.text(x-width/2, y, value, size, color, bold)

    def box(self, rect, fill=PANEL, border=LINE, width=2, radius=24):
        self.d.rounded_rectangle(rect, radius=radius, fill=fill, outline=border, width=width)

    def arrow(self, start, end, color=CYAN, width=5):
        self.d.line((start, end), fill=color, width=width)
        angle = math.atan2(end[1]-start[1], end[0]-start[0])
        points = [end]
        for offset in (-0.5, 0.5):
            points.append((end[0]-20*math.cos(angle+offset), end[1]-20*math.sin(angle+offset)))
        self.d.polygon(points, fill=color)

    def save(self, name):
        self.im.save(OUT / name, optimize=True)
        print(name)


def overview():
    f = Figure('01 / PROJECT', '환경과 학습을 나누는 두 프로세스', 'Unreal은 경험을 만들고, Python은 그 경험으로 행동 가치를 갱신합니다.')
    for x, label, color in [(80, 'UNREAL ENGINE', CYAN), (1160, 'PYTHON', AMBER)]:
        f.box((x, 300, x+560, 735), border=color)
        f.text(x+38, 336, label, 42, color, True)
    f.text(118, 430, 'Agent.cpp  ·  TargetPositioner.cpp', 25, MUTED)
    f.text(118, 500, '01   액터 배치 및 이동\n02   상태 · 다음 상태 계산\n03   보상 · 에피소드 종료 판정', 29)
    f.text(1198, 430, 'main.py  ·  PyTorch', 25, MUTED)
    f.text(1198, 500, '01   epsilon-greedy 행동 선택\n02   Q-table 갱신\n03   model.txt 저장', 29)
    f.box((700, 410, 1100, 610), fill='#172936')
    f.center(900, 446, 'SHARED MEMORY', 28, WHITE, True)
    f.center(900, 500, '20 bytes + Named Events', 23, MUTED)
    f.arrow((640, 465), (700, 465))
    f.arrow((1100, 465), (1160, 465))
    f.arrow((1160, 565), (1100, 565), AMBER)
    f.arrow((700, 565), (640, 565), AMBER)
    f.center(900, 338, '상태 / 보상 →', 25, CYAN)
    f.center(900, 662, '← 선택한 행동', 25, AMBER)
    f.center(900, 809, '환경 실행과 Q값 계산은 별도 프로세스에서, 한 스텝씩 동기화됩니다.', 29)
    f.save('01-project-overview.png')


def runtime():
    f = Figure('02 / RUNTIME', '같은 Windows 세션에서 실행', '로컬 공유 메모리와 kernel32 이벤트로 연결되는 실행 환경입니다.')
    f.box((80, 280, 1720, 850), border=CYAN)
    f.text(120, 310, 'WINDOWS  /  LOCAL SESSION', 30, CYAN, True)
    cards = [(120, 'UNREAL C++', '호환 엔진 + C++ 빌드 도구', '게임 환경\n액터 이동 · 보상 계산', CYAN),
             (650, 'PYTHON 3.11', 'uv sync  /  uv run main.py', 'PyTorch 2.5.1\nCPU Q-table · 5 × 3', AMBER),
             (1180, 'WINDOWS IPC', 'kernel32', 'Named Shared Memory\nNamed Events', CYAN)]
    for x, title, sub, body, color in cards:
        f.box((x, 402, x+490, 735))
        f.text(x+28, 438, title, 35, color, True)
        f.text(x+28, 503, sub, 23, MUTED)
        f.text(x+28, 586, body, 28)
    f.center(900, 781, 'CUDA 패키지 인덱스를 사용하지만, 현재 학습 코드는 GPU를 사용하지 않습니다.', 25, MUTED)
    f.save('02-runtime.png')


def startup():
    f = Figure('03 / STARTUP', '환경을 준비한 뒤 학습기를 연결', '설치는 먼저, 프로세스 연결은 Unreal → Python 순서로 진행합니다.')
    steps = [('01', 'Python 준비', 'uv sync', 'RL_T3_PY 폴더', CYAN),
             ('02', 'Unreal 시작', 'BeginTrain(...)', '목표 액터 연결 · 양수 인수', CYAN),
             ('03', '준비 확인', 'Waiting for brain.', 'Unreal Output Log', CYAN),
             ('04', '학습 시작', 'uv run main.py', '상태 → 행동 → 보상 → 갱신', AMBER)]
    for i, (num, title, cmd, note, color) in enumerate(steps):
        x = 80+i*420
        f.box((x, 320, x+380, 710), border=color)
        f.text(x+28, 350, num, 55, color, True)
        f.text(x+28, 440, title, 34, WHITE, True)
        f.text(x+28, 521, cmd, 26, color)
        f.text(x+28, 601, note, 21, MUTED)
        if i < 3:
            f.arrow((x+384, 505), (x+414, 505), color, 3)
    f.box((80, 765, 1720, 862))
    f.center(900, 792, '정상 종료 → data/model.txt 저장     |     다시 실행하면 Q-table은 0부터 시작', 27)
    f.save('03-startup-sequence.png')


def states():
    f = Figure('05 / STATES & ACTIONS', '다섯 상태, 세 가지 행동', 'Distance = Target.X − Agent.X  ·  숫자 축은 목표의 상대 거리입니다.')
    labels = [('0', '멀리 왼쪽', 'd ≤ −500', '←', CYAN),
              ('1', '가까이 왼쪽', '−500 < d ≤ −100', '←', CYAN),
              ('2', '목표 구간', '−100 < d ≤ 100', '■', AMBER),
              ('3', '가까이 오른쪽', '100 < d ≤ 500', '→', CYAN),
              ('4', '멀리 오른쪽', '500 < d', '→', CYAN)]
    for i, (state, name, interval, action, color) in enumerate(labels):
        x = 80+i*332
        f.box((x, 300, x+312, 640), border=color)
        f.text(x+25, 328, 'STATE '+state, 25, color, True)
        f.center(x+156, 390, name, 29, WHITE, True)
        f.center(x+156, 449, interval, 24, MUTED)
        f.center(x+156, 510, action, 72, color, True)
    for x, title, val, color in [(80, 'ACTION 0 / 왼쪽', '로컬 X −10', CYAN), (640, 'ACTION 1 / 정지', '이동 없음', AMBER), (1200, 'ACTION 2 / 오른쪽', '로컬 X +10', CYAN)]:
        f.box((x, 702, x+520, 840))
        f.text(x+26, 726, title, 27, color, True)
        f.text(x+26, 781, val, 27)
    f.save('05-states-actions.png')


def rewards():
    f = Figure('06 / REWARDS', '목표 구간에서 정지해야 성공', '종료 조건을 먼저 판정합니다. 마지막 허용 스텝에서도 성공하면 +20입니다.')
    cards = [(80, 290, '+20', '성공 · 종료', '목표 구간에서 정지 행동', AMBER),
             (920, 290, '−10', '실패 · 종료', '마지막 스텝까지 성공하지 못함', RED),
             (80, 590, '+1', '접근 · 계속', '종료 조건 없이 절대 거리 감소', CYAN),
             (920, 590, '−1', '그 외 · 계속', '멀어짐 / 목표 밖 정지 등', MUTED)]
    for x, y, value, title, desc, color in cards:
        f.box((x, y, x+800, y+260), border=color)
        f.text(x+35, y+48, value, 83, color, True)
        f.text(x+250, y+49, title, 36, WHITE, True)
        f.text(x+250, y+130, desc, 27, MUTED)
    f.save('06-rewards.png')


def learning():
    f = Figure('07 / Q-LEARNING', '경험으로 Q값을 목표값에 가깝게', 'Q값은 성공 확률이 아니라 할인된 미래 보상을 포함한 행동 가치입니다.')
    f.box((80, 280, 1720, 405))
    f.text(115, 318, '행동 선택', 31, WHITE, True)
    f.box((340, 308, 690, 379), fill='#19404b', border=CYAN, radius=14)
    f.center(515, 324, '20%  무작위 선택', 28, CYAN, True)
    f.box((715, 308, 1675, 379), fill='#3b3023', border=AMBER, radius=14)
    f.center(1195, 324, '80%  현재 Q값이 가장 큰 행동', 28, AMBER, True)
    f.box((80, 450, 880, 655), border=CYAN)
    f.text(115, 477, '일반 전이', 28, CYAN, True)
    f.text(115, 540, 'target = r + 0.9 × max Q(s′, a′)', 34)
    f.box((920, 450, 1720, 655), border=AMBER)
    f.text(955, 477, '종료 전이  /  +20 또는 −10', 28, AMBER, True)
    f.text(955, 540, 'target = r', 38)
    f.arrow((480, 665), (480, 707))
    f.arrow((1320, 665), (1320, 707), AMBER)
    f.box((80, 720, 1720, 866), fill='#192b39', border=CYAN)
    f.center(900, 743, 'Q(s, a) ← Q(s, a) + 0.1 × [target − Q(s, a)]', 39, WHITE, True)
    f.center(900, 809, 'alpha = 0.1     ·     gamma = 0.9     ·     epsilon = 0.2', 24, MUTED)
    f.save('07-qlearning-update.png')


def ipc():
    f = Figure('08 / COMMUNICATION', '이벤트로 맞추는 한 스텝', '시작 신호는 한 번, 학습 중에는 2–5번을 반복합니다. 같은 Windows 세션에서 통신합니다.')
    lx, rx = 300, 1500
    f.center(lx, 265, 'UNREAL', 34, CYAN, True)
    f.center(rx, 265, 'PYTHON', 34, AMBER, True)
    f.d.line((lx, 330, lx, 730), fill=LINE, width=4)
    f.d.line((rx, 330, rx, 730), fill=LINE, width=4)
    events = [('1   BrainStartEvent', False), ('2   StateSettedEvent', True), ('3   ActionSettedEvent', False), ('4   RewardSettedEvent', True), ('5   TrainedCompEvent', False)]
    for i, (name, to_python) in enumerate(events):
        y = 355+i*82
        color = CYAN if to_python else AMBER
        f.arrow((lx, y) if to_python else (rx, y), (rx, y) if to_python else (lx, y), color, 3)
        f.box((620, y-25, 1180, y+25), fill=BG, border=BG, radius=4)
        f.center(900, y-21, name, 26, color, True)
    f.text(80, 760, 'Local\\AgentMemory  /  20 bytes', 25, WHITE, True)
    fields = [('State', '0–3'), ('NextState', '4–7'), ('Action', '8–11'), ('Reward', '12–15'), ('IsTraining', '16–19')]
    for i, (name, offset) in enumerate(fields):
        x=80+i*332
        f.box((x, 808, x+312, 898), radius=12)
        f.center(x+156, 815, name, 25, WHITE, True)
        f.center(x+156, 856, offset+' bytes', 19, MUTED)
    f.save('08-ipc-sequence.png')


def results():
    f = Figure('09 / RESULTS', '각 상태의 최댓값이 만드는 정책', '공유된 학습 결과 예시 · 소수 셋째 자리 표시 · 별도 성공률 벤치마크가 아닙니다.')
    values = [[10.391194343566895,8.171244621276855,8.061776161193848],
              [12.962371826171875,9.404415130615234,9.369195938110352],
              [11.268392562866211,19.99999237060547,11.831381797790527],
              [8.410754203796387,8.841075897216797,10.420808792114258],
              [8.216452598571777,8.225564002990723,10.314126968383789]]
    policy = [max(range(3), key=lambda a: row[a]) for row in values]
    assert policy == [0, 0, 1, 2, 2]
    for i, label in enumerate(['왼쪽 / 0', '정지 / 1', '오른쪽 / 2']):
        f.center(510+i*300, 271, label, 29, WHITE, True)
    labels=['멀리 왼쪽', '가까이 왼쪽', '목표 구간', '가까이 오른쪽', '멀리 오른쪽']
    for s, row in enumerate(values):
        y=335+s*102
        f.text(80, y+8, 'STATE '+str(s), 23, CYAN, True)
        f.text(80, y+46, labels[s], 22, MUTED)
        for a, value in enumerate(row):
            x=360+a*300
            selected=a==policy[s]
            t=(value-7)/13
            fill=(int(18+10*t),int(37+45*t),int(51+52*t))
            f.box((x,y,x+280,y+86), fill=fill, border=AMBER if selected else LINE, width=4 if selected else 2, radius=14)
            f.center(x+140,y+19,f'{value:.3f}',36,AMBER if selected else WHITE,selected)
    f.box((1305, 335, 1720, 831), border=AMBER)
    f.center(1512, 367, 'GREEDY POLICY', 27, AMBER, True)
    for i, label in enumerate(['←  LEFT', '←  LEFT', '■  STOP', '→  RIGHT', '→  RIGHT']):
        f.center(1512, 442+i*68, label, 32, WHITE, True)
    f.text(80, 866, '금색 테두리 = 행의 최댓값    |    정책 평가 시 Q값 갱신 중지 + epsilon = 0', 25, MUTED)
    f.save('09-qtable-results.png')


def limitations():
    f = Figure('10 / LIMITS', '올바른 정책과 성공 가능한 환경', '성공률은 행동 선택뿐 아니라 시작 거리, 이동량, 남은 스텝에도 영향을 받습니다.')
    f.box((80, 290, 1000, 825), border=CYAN)
    f.text(115, 325, '예시: 목표가 오른쪽으로 1000유닛', 33, WHITE, True)
    f.center(540, 417, '1000 → 100', 68, CYAN, True)
    f.center(540, 520, '10유닛씩 이동 × 90회', 35)
    f.center(540, 587, '+ 목표 구간에서 정지 1회', 32)
    f.box((125, 684, 955, 781), fill='#3b3023', border=AMBER)
    f.center(540, 709, '최소 91스텝  >  기본 제한 50스텝', 34, AMBER, True)
    f.box((1040, 290, 1720, 825))
    f.text(1075, 333, '같은 상태에도 다른 상황', 34, WHITE, True)
    f.text(1075, 417, '넓은 거리 구간을 하나의 행으로 표현\n남은 스텝 수는 상태에 포함되지 않음', 27, MUTED)
    f.d.line((1075, 555, 1685, 555), fill=LINE, width=2)
    f.text(1075, 594, '평가할 때 확인할 항목', 31, CYAN, True)
    f.text(1075, 662, '성공률 · 평균 스텝 · 시작 거리별 실패\n모델 로딩 / 평가 모드는 추가 구현 필요', 26)
    f.save('10-evaluation-limits.png')


if __name__ == '__main__':
    for build in [overview, runtime, startup, states, rewards, learning, ipc, results, limitations]:
        build()
