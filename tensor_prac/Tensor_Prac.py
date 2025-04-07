"""
This file has no actual content related to tranformers or the mock GPT. This
just is some practice for myself with python, and the tensors in pytorch.
"""

import os.path

import torch


def main():
    """This will open the file and is a doxygen test"""
    with open(os.path.dirname(__file__) + "/../input.txt", "r", encoding="utf-8") as f:
        text = f.read()

    print("length of dataset in characters: ", len(text))

    print(text[:1000])

    chars = sorted(list(set(text)))
    vocab_size = len(chars)
    print("".join(chars))
    print(vocab_size)

    """ create a mapping from characters to integers """
    stoi = {ch: i for i, ch in enumerate(chars)}
    itos = {i: ch for i, ch in enumerate(chars)}
    """ encoder: takes a string, outputs a list of integers """
    encode = lambda s: [stoi[c] for c in s]
    """ decoder: take a list of strings, output the string """
    decode = lambda l: "".join([itos[i] for i in l])

    print(encode("test message"))
    print(decode(encode("test message")))

    """ create a tensor representation of the encoding for all the text"""
    data = torch.tensor(encode(text), dtype=torch.long)
    print(data.shape, data.dtype)
    """ print the first 1000 elements of tensor """
    print(data[:1000])

    """ get the train/val split data """
    train_data, val_data = train_val_split(data)


def print_block_size(block_size, train_data):
    """
    This function will be used to print the block size for the data.
    Another way to describe block size is the amount of context that
    we will look at for each pass for our model. So in this case the
    block_size could be variable but for this we will fix it to 8 for
    this practice model

    @param block_size: the size of context we want to take in
    @param train_data: the training data we are using
    @return: void there is nothing returned
    """
    print(train_data[: block_size + 1])


def train_val_split(data):
    """
    Function that will create a 90% split between the data for testing and validation

    @param data: the data that needs to be split
    @return: the training and validation data tensors
    """

    n = int(0.9 * len(data))
    train_data = data[:n]
    val_data = data[n:]
    return train_data, val_data


main()
