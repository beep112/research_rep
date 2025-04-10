import os
import os.path
import sys

# Add the parent directory (root_directory) to the Python path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import torch
import torch.nn as nn
from torch.nn import functional as F

import tensor_prac.Tensor_Prac as tp

# hyperparameters (used to set things about the model)
batch_size = 256
block_size = 8
max_iters = 1000000
eval_interval = 100
learning_rate = 1e-2
# check for mps which is Apple's Metal Performance Shaders which
# has a backend in pytorch this should allow the GPU for M-series macs to be
# used
"""
if torch.backends.mps.is_available() and torch.backends.mps.is_built():
    device = "mps"
# check for cuda which should be used because obviously
elif torch.cuda.is_available():
    device = "cuda"
# if now GPU (AMD excluded right now) then just use the CPU
else:
"""

device = "cpu"
print(f"Using device: {device}")
eval_iters = 200

"""
@file Bigram.py
@brief Practice with PyTorch tensors and making shakepeare llm with bigram

This file contains practice code for working with PyTorch tensors and 
implemenation of a bigram with respective functions. Will be using PyTorch
functions to build this Bigram.

The context for the Bigram is only the last character in this case, but could
we add more context than this?

@author Jason Perndreca
@version %I%, %G%
@note accesses functions in tensor_prac that do functions that were alreeady create as functions
@todo Need to add the ability for GPU to be used and also try to use MLP for Macs
"""


def main():
    torch.manual_seed(1337)
    input_path = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "input.txt")
    )
    with open(input_path, "r", encoding="utf-8") as f:
        text = f.read()
    chars = sorted(list(set(text)))
    vocab_size = len(chars)

    # create a mapping from characters to integers
    stoi = {ch: i for i, ch in enumerate(chars)}
    # create the reverse mapping taking an integer to a string
    itos = {i: ch for i, ch in enumerate(chars)}

    encode = lambda s: [stoi[c] for c in s]
    decode = lambda l: "".join([itos[i] for i in l])

    # create a tensor representing the encoding of all the text
    data = torch.tensor(encode(text), dtype=torch.long)

    train_data, val_data = tp.train_val_split(data)
    input, targs = get_batch("train", train_data, val_data)

    model = BigramLanguageModel(vocab_size)
    # IMPORTANT: Need to load to something like a GPU using .to(device) on the model
    m = model.to(device)

    context = torch.zeros((1, 1), dtype=torch.long, device=device)
    # before training the model generating tokens will split out complete
    # nonsense since the weights are random
    print("Before training 100 generated tokens")
    print(decode(m.generate(context, max_new_tokens=1000)[0].tolist()))

    # TRAINING WORK
    print("Training loss values")
    optimizer = torch.optim.AdamW(model.parameters(), lr=learning_rate)
    for step in range(max_iters):

        # every so often need to evaluate the loss and train data set
        if step % eval_interval == 0:
            losses = estimate_loss(model, train_data, val_data)
            print(
                f"step {step}: train loss {losses['train']:.4f}, val loss{losses['val']:.4f}"
            )

        # sample batch of data
        input, targs = get_batch("train", train_data, val_data)

        # evaluate the loss
        logits, loss = model(input, targs)
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        optimizer.step()

    print("After training 100 generated tokens")
    print(decode(m.generate(context, max_new_tokens=1000)[0].tolist()))


def get_batch(split, train_data, val_data):
    """
    @brief new version of get_batch that allows for model to be loaded to gpus

    The previous version of get_batch in the Tensor_Prac doesn't have the ability
    for the GPU to actualy be used, even if the device is present. So this new
    version will take advantage of that hardware.

    @param split: string either 'train' or 'val' which is used to pick data to batch with
    @param train_data: the data that is split for training
    @param val_data: the data that is split for validation
    @return Tuple(Tensor of inputs, Tensor or targets)
    """
    # generate a small batch of data of inputs x and targets y
    data = train_data if split == "train" else val_data
    ix = torch.randint(len(data) - block_size, (batch_size,))
    x = torch.stack([data[i : i + block_size] for i in ix])
    y = torch.stack([data[i + 1 : i + block_size + 1] for i in ix])
    # IMPORTANT: This is the part that is needed for the GPU
    x, y = x.to(device), y.to(device)
    return x, y


@torch.no_grad()
def estimate_loss(model, train_data, val_data):
    out = {}
    model.eval()
    for split in ["train", "val"]:
        losses = torch.zeros(eval_iters)
        for k in range(eval_iters):
            X, Y = get_batch(split, train_data, val_data)
            logits, loss = model(X, Y)
            losses[k] = loss.item()
        out[split] = losses.mean()
    model.train()
    return out


class BigramLanguageModel(nn.Module):

    def __init__(self, vocab_size):
        """
        @brief constructor for BigramLanguageModel that calls super, and then
        creates a new embedding table of vocab_size by vocab_size

        @param self: the current BigramLanguageModel object
        @param vocab_size: the length of the vocab_size
        @return new BigramLanguageModel
        """
        super().__init__()
        # each token directly reads off the logits for hte next token from a tookup table
        self.token_embedding_table = nn.Embedding(vocab_size, vocab_size)

    def forward(self, idx, targets=None):
        """
        @brief goes "ahead" by getting our next set of logits and then returns
        the logits and loss

        @param self: the current BigramLanguageModel object
        @param idx: the current context batch
        @param targets: the tokens that are actually next
        @return logits, loss: Tuple(logits, loss), which are the logits, and
        the loss under cross_entropy
        """

        # idx and targets are both (B, T) tensor of integers
        logits = self.token_embedding_table(idx)  # (B, T, C)

        # negative log likelyhood loss (cross-entropy) how close are logits
        # to target
        if targets == None:
            loss = None
        else:
            B, T, C = logits.shape
            logits = logits.view(B * T, C)
            targets = targets.view(B * T)
            loss = F.cross_entropy(logits, targets)

        return logits, loss

    def generate(self, idx, max_new_tokens):
        """
        @brief this is the generator of the bigram model, where the model creates tokens

        Idx is the current context of characters in the current batch. And we
        expent all the B dimension in the T dimension. What is predicted along
        the T dimension.
        We take the current idx, focus on the last element in the T dimension.
        These are then converted to probabilities using softmax, and the sampled
        using multinomial. We then take the sampled integers, and concat them

        @param self: the current BigramLanguageModel object
        @param idx: the current context batch (which is dim(B by T))
        @param max_new_tokens: the amount of new tokens we want to generate
        @return idx: modified idx with the new generated tokens appended
        """
        # idx is (B, T) array of incixes in the current context
        for _ in range(max_new_tokens):
            # get the predictions
            logits, loss = self(idx)
            # focus only on the last time step
            logits = logits[:, -1, :]  # now dim(B by C)
            # apply the softmax to get probabilities
            probs = F.softmax(logits, dim=-1)  # dim(B by C)
            # sample from the distribution
            idx_next = torch.multinomial(probs, num_samples=1)  # (B, 1)
            # append sampled index to the running sequence
            idx = torch.cat((idx, idx_next), dim=1)  # dim(B by T-1)
        return idx


if __name__ == "__main__":
    main()
