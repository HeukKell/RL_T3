import torch
import ctypes # ctype 을 쓸 수 있도록 하는 라이브러리 . windows type 도 ctype 이다.
from multiprocessing import shared_memory

import struct
import time
import random



from datetime import datetime
from pathlib import Path
import inspect

LOG_FILE_PATH = Path(f"logs.txt")

def PLOG(msg : str, writeFile : bool = False):
    caller = inspect.stack()[1]

    filepath = Path(caller.filename)
    filename = filepath.name
    lineno = caller.lineno
    funcname = caller.function

    now = datetime.now().strftime("%Y%m%d.%H:%M:%S.%f")[:-3]

    debug_msg = f"[{now}] {filename}({lineno}).{funcname} >> {msg}"

    print(debug_msg)

    if writeFile:
        LOG_FILE_PATH.parent.mkdir(parents = True, exist_ok = True)

        with open(LOG_FILE_PATH, "a", encoding="utf-8") as file:
            file.write(debug_msg + "\n")



SHARED_MEMORY_NAME = "Local\\AgentMemory"
SHARED_MEMORY_SIZE = 20

shared = None

class SharedMemoryType() :
    def __init__(self):

        self.shared = None 

        try : 
            self.shared = shared_memory.SharedMemory(
                name = SHARED_MEMORY_NAME,
                create = False, 
                size = SHARED_MEMORY_SIZE

            )
        except Exception as e :
            PLOG(f"공유메모리 연결에 실패하였습니다.{e}")


        if self.shared != None :
            PLOG(f"공유 메모리 연결 완료")

        else :
            PLOG("공유메모리 연결 실패")

    def get_state(self) -> int :
        # 상태를 반환한다
        state = struct.unpack("<i",self.shared.buf[0:4])
        return state
    def get_nextState(self)  -> int:
        # 다음 상태를 반환한다
        state = struct.unpack("<i", self.shared.buf[4:8])
        return state

    def get_action(self) -> int:
        # 액션을 반환합니다.
        action = struct.unpack("<i", self.shared.buf[12:16])
        return action

    def set_action(self, newAction : int):
        self.shared.buf[8:12] = struct.pack("<i", newAction)
        return

    def get_reward(self) -> float :
        reward = struct.unpack("<f", self.shared.buf[12:16])
        return reward

    def get_isTraining(self) -> float :
        training = struct.unpack("<i", self.shared.buf[16:20])
        return training

    def set_isTraining(self) :
        self.shared.buf[16:20] = struct.pack("<i", 1)
        return

    def close(self) :
        if self.shared != None :
            # 현재 python 에서 연결 해제
            self.shared.close()

            # 운영체제에서 공유메모리 이름 제거. 근데 이건 unreal 에서 해야하지 않나 싶다.
            self.shared.unlink()

def main():
    print("Hello from rl-t3-py!")

    # 1. 공유메모리 연결
    # 2. Named Event 연결
    # 3. QTable 생성
    # 4. SetEvent(BrainStartEvent) ---> 

    # 5. WaitForSingleObject(StateSettedEvent)      <---
    # 6. QTable 에서 Epsilon 0.2 주고, Action 얻어내기
    # 7. SetEvent(ActionSettedEvent) --->
    # 8. WaitForSingleObject(RewardSettedEvent) <---
    # 9. Reward 가지고 Target 이랑, 학습 적용
    # 10. 5번으로 이동

    shared_memory = None

    q_table = torch.zeros(5,3) # 3개짜리 5개

    print(q_table)

    endTimer = 0.0

    # ctype wind dll 라이브러리 연결
    kernel32 = ctypes.WinDLL("kernel32", use_last_error= True)

    # dll 함수의 시그니처 선언
    # dll 함수라서, 시그니처를 알아야 하기 때문에 아래와같이 인수와 반환값을 설정해주어야 한다.

    kernel32.OpenEventW.argtypes = [
        ctypes.c_uint32,
        ctypes.c_int,
        ctypes.c_wchar_p,
    ]

    kernel32.OpenEventW.restype = ctypes.c_void_p

    kernel32.WaitForSingleObject.argtypes = [
        ctypes.c_void_p,
        ctypes.c_uint32,
    ]

    kernel32.WaitForSingleObject.restype = ctypes.c_uint32

    kernel32.CloseHandle.argtypes = [ctypes.c_void_p]

    # ---

    SYNCHRONIZE = 0x00100000
    INFINITE = 0xFFFFFFF

    brainStartEvent= kernel32.OpenEventW(SYNCHRONIZE, False, "Lcoal\\BrainStartEvent")
    stateSettedEvent = kernel32.OpenEventW(SYNCHRONIZE, False, "Local\\StateSettedEvent")
    actionSettedEvent = kernel32.OpenEventW(SYNCHRONIZE, False, "Local\\ActionSettedEvent")
    rewardSettedEvent = kernel32.OpenEventW(SYNCHRONIZE, False, "Local\\RewardSettedEvent")
    trainCompEvent = kernel32.OpenEventW(SYNCHRONIZE, False, "Local\\TrainCompEvent")
    

    epsilon = 0.2;
    
    try : 

        shared_memory = SharedMemoryType()

        # BrainStartEvent 알려야 한다
        kernel32.SetEvent(brainStartEvent)
        while True:

            # state 가 설정될때까지 대기
            kernel32.WaitForSingleObject(stateSettedEvent, INFINITE)
            # state 가 설정 되었다

            # state 를 읽고 행동 설정하기
            currentState = shared_memory.get_state()
            action = -1
            if epsilon < random.random() :
                # 탐험 로직
                action = random.randint(0,2)
                
            else :
                action = q_table[currentState].argmax().item()

            # action 설정
            shared_memory.set_action(action)
            kernel32.SetEvent(actionSettedEvent)

            # 보상 올때까지 대기
            kernel32.WaitForSingleObject(rewardSettedEvent, INFINITE)
            # 보상 왔다

            reward = shared_memory.get_reward()
            state = shared_memory.get_state()
            nextState = shared_memory.get_nextState()
            action = shared_memory.get_action()

            gamma = 0.9
            target = 0
            alpha = 0.1

            if reward == 20 :
                # 완료
                target = reward
            else :
                # 현재의 보상 + 미래상태의 가치
                target = reward + gamma*q_table[nextState].max().item()


            q_table[currentState, action] += q_table[currentState,action].item() + alpha * target

            print("학습 결과 : ")
            print(q_table)

            kernel32.SetEvent(trainCompEvent)

            # 아주 잠깐 학습 종료했는지 확인하고 넘어간다.
            
            if not shared_memory.get_isTraining() == 1 :
                # 학습 종료
                print("학습 종료")
                print(q_table)

                with open("data/model.txt", "wt",encoding="utf-8") as file :

                    model_flatten = q_table.flatten()

                    for idx in range(15) :
                        
                        file.write(model_flatten[idx].item())
                        
                        if idx % 3 == 1:
                            file.write("\n")
                        else:
                            if idx != 14 :
                                file.write(",")

                    print("모델 출력완료")
                break;
            

    except Exception as e :
        PLOG(e)

    finally :
        shared_memory.close()

        kernel32.CloseHandle(brainStartEvent)
        kernel32.CloseHandle(stateSettedEvent)
        kernel32.CloseHandle(actionSettedEvent)
        kernel32.CloseHandle(rewardSettedEvent)
        kernel32.CloseHandle(trainCompEvent)

        print("프로그램 종료")        

if __name__ == "__main__":
    main()
