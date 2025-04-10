import os

import torch
import torch.nn as nn
from torch.nn import functional as F

# VERSION 1: Naive approach using loops
torch.manual_seed(1337)
B, T, C = 4, 8, 2
# we want the tokens to talk to previous tokens, to fifth token talks to
# 4th, 3rd, 2nd, 1st, token
x = torch.randn(B, T, C)
x.shape
# we can use average even though it is lossy better version in transformer
xbow = torch.zeros((B, T, C))
for b in range(B):
    for t in range(T):
        xprev = x[b, : t + 1]  # (t, C)
        xbow[b, t] = torch.mean(xprev, 0)
print(x[0])
print(xbow[0])


# VERSION 2: Using Matrix Multiplication
# This matrix math is the same thing as above but better for computers

# we can make a weights matrix for eveery things we are trying to normalize
wei = torch.tril(torch.ones(T, T))
wei = wei / wei.sum(1, keepdim=True)
xbow2 = wei @ x  # (B, T, T) @ (B, T, C) ----> (B, T, C) xbow2 = xbow now
torch.allclose(xbow, xbow2)
print(f"xbow1: {xbow} \n xbow2: {xbow2}")


# VERSION 3: Using Softmax
tril = torch.tril(torch.ones(T, T))
wei = torch.zeros((T, T))
wei = wei.masked_fill(tril == 0, float("-inf"))
wei = F.softmax(wei, dim=-1)
xbow3 = wei @ x
print(f"xbow1: {xbow} \n xbow2: {xbow2} \n xbow3: {xbow3}")

# We dont want the current version because this is constant across all numbers
"""
self-attension fixes this, every single token will create two vectors
queries, and keys vector, the queries is what am I looking for? and the
keys is what do I contain? The way that we get context betwen tokens is by doing
the dot-product between all the keys and queries.
So we will take the max context
"""
# VERSION 4: Self-attention!!!!!!
torch.manual_seed(1337)
B, T, C = 4, 8, 32  # batch, time, channels
x = torch.randn(B, T, C)

# A single Head performing self-attention
head_size = 16
key = nn.Linear(C, head_size, bias=False)
query = nn.Linear(C, head_size, bias=False)
k = key(x)  # (B, T, 16)
q = query(x)  # (B, T, 16)
wei = q @ k.transpose(-2, -1)  # (B, T, 16) @ (B, 16, T) ---> (B, T, T)
# now have a T by T matrix that is the attention
tril = torch.tril(torch.ones(T, T))
wei = wei.masked_fill(tril == 0, float("-inf"))
wei = F.softmax(wei, dim=-1)
out = wei @ x
print("Self Attention")
print(out.shape)
print(wei)
print(out)

# we can make this a lot more efficient using matrix multiplication
torch.manual_seed(42)
a = torch.tril(torch.ones(3, 3))
a = a / torch.sum(a, 1, keepdim=True)
b = torch.randint(0, 10, (3, 2)).float()
# the (@) operator will multiply two matrices in pytorch
c = a @ b
print("a=")
print(a)
print("b=")
print(b)
print("---")
print(c)
