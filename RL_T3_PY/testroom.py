import random
import torch


tensor_1 =torch.randint(0,100, (4,3))
print(tensor_1)

print(tensor_1[1].max().item())

# for idx in range(20) :
#     print(random.random())