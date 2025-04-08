"""
@file Tensor_Prac.py
@brief Practice with PyTorch tensors and basic transformer concepts.

This file contains practice code for working with PyTorch tensors and basic data processing
for transformer models, including data loading, encoding/decoding, and batch creation.

@author Jason Perndreca
@version %I%, %G%
"""

import os.path

import torch


def main():
    """
    @brief Main function that loads data and demonstrates basic tensor operations.

    Opens a text file, processes the data, creates training/validation splits,
    and demonstrates block size operations.

    @return None
    """

    # ensure that when data is randomly sampled we can reproduce that result
    torch.manual_seed(1337)
    with open(os.path.dirname(__file__) + "/../input.txt", "r", encoding="utf-8") as f:
        text = f.read()

    print("length of dataset in characters: ", len(text))

    print(text[:1000])

    chars = sorted(list(set(text)))
    vocab_size = len(chars)
    print("".join(chars))
    print(vocab_size)

    # create a mapping from characters to integers
    stoi = {ch: i for i, ch in enumerate(chars)}
    itos = {i: ch for i, ch in enumerate(chars)}
    # encoder: takes a string, outputs a list of integers
    encode = lambda s: [stoi[c] for c in s]
    # decoder: take a list of strings, output the string
    decode = lambda l: "".join([itos[i] for i in l])

    print(encode("test message"))
    print(decode(encode("test message")))

    # create a tensor representation of the encoding for all the text
    data = torch.tensor(encode(text), dtype=torch.long)
    print(data.shape, data.dtype)
    # print the first 1000 elements of tensor
    print(data[:1000])

    # get the train/val split data
    train_data, val_data = train_val_split(data)
    print_block_size(8, train_data)
    input, targs = get_batch(4, 8, "train", train_data, val_data)
    print_training_batch(input, targs, 4, 8)


def print_block_size(block_size, train_data):
    """
    @brief Prints examples of context-target pairs for given block size.

    Demonstrates how the context window works by showing input-output pairs
    for different context lengths.

    This function will be used to print the block size for the data.
    Another way to describe block size is the amount of context that
    we will look at for each pass for our model. So in this case the
    block_size could be variable but for this we will fix it to 8 for
    this practice model

    @param block_size: the size of context we want to take in
    @param train_data: the training data we are using
    @return None
    """
    print(train_data[: block_size + 1])

    # x will be used at training data and compared against y which is the
    # actual next token
    x = train_data[:block_size]
    y = train_data[1 : block_size + 1]
    for t in range(block_size):
        context = x[: t + 1]
        target = y[t]
        print(f"when input is {context} the target: {target}")


def train_val_split(data):
    """
    @brief Splits data into training and validation sets.

    Creates a 90%/10% split between training and validation data.

    @param data: Full dataset tensor
    @return Tuple of (train_data, val_data)
    """

    n = int(0.9 * len(data))
    train_data = data[:n]
    val_data = data[n:]
    return train_data, val_data


def get_batch(batch_size, block_size, split, train_data, val_data):
    """
    @brief creates a training batch that can be stacked (better for gpus)

    When training data we can create a batch of block sizes to train on which is better for
    GPUs as they want to process multiple calcuations in one input

    @param batch_size: the size of the batch (how many independent sequences will we process in parallel)
    @param block_size: the size of the context (what is the maximum context length for predictions)
    @param train_data: the Tensor that stores the data used to train
    @param val_data: the Tensor that stores the data used to validate
    @return Tuple(Tensor(batch_inputs), Tensor(batch_targets))

    """
    data = train_data if split == "train" else val_data
    # randint will generate a random location for training
    ix = torch.randint(len(data) - block_size, (batch_size,))
    # stack can be used to append a tensor to another
    x = torch.stack([data[i : i + block_size] for i in ix])
    y = torch.stack([data[i + 1 : i + block_size + 1] for i in ix])
    return x, y


def print_training_batch(input, targs, batch_size, block_size):
    """
    @brief creates a training batch that can be stacked (better for gpus)

    When training data we can create a batch of block sizes to train on which is better for
    GPUs as they want to process multiple calcuations in one input

    @param input: the input batch tensor
    @param targs: the output batch tensor
    @param batch_size: the batch_size of our input tensors
    @param block_size: the batch_size of our model
    @return None

    """
    print()
    print("inputs:")
    print(input.shape)
    print(input)
    print("targets:")
    print(targs.shape)
    print(targs)
    print("---------")
    # this is how we would traverse this batch tensor (it's basically a matrix)
    for b in range(batch_size):
        for t in range(block_size):
            context = input[b, : t + 1]
            target = targs[b, t]
            print(f"when input is {context.tolist()} the target: {target}")

    # seed for this because we want to make it random but reproducable


if __name__ == "__main__":
    main()
