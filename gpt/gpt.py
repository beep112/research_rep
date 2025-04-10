import os

import torch
from torch.nn import functional as F

# hyperparameters
batch_size = 64  # how many independent sequences will we process in parallel
block_size = 256  # maximum context length of predicting
max_iters = 5000
eval_interval = 500
learning_rate = 3e-4
if torch.backends.mps.is_available() and torch.backends.mps.is_built():
    device = "mps"
# check for cuda which should be used because obviously
elif torch.cuda.is_available():
    device = "cuda"
# if now GPU (AMD excluded right now) then just use the CPU
else:
    device = "cpu"
eval_iters = 200
n_embd = 384
n_head = 6
n_layer = 6
dropout = 0.2
# -----------------------------------------------------------------------------

torch.manual_seed(1337)

input_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "input.txt"))
with open(input_path, "r", encoding="utf-8") as f:
    text = f.read()

# all the unique characters that occur in the text
chars = sorted(list(set(text)))
vocab_size = len(chars)

# create a mapping for characters to integers
stoi = {ch: i for i, ch in enumerate(chars)}
itos = {i: ch for i, ch in enumerate(chars)}
encode = lambda s: [stoi[c] for c in s]
decode = lambda l: "".join([itos[i] for i in l])

# Train and test splits
