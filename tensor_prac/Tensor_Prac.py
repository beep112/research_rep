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


def train_val_split(data):
    """
    Function that will create a 90% split between the data for testing and validation

    @param the data that needs to be split
    """

    n = int(0.9 * len(data))
    train_data = data[:n]
    val_data = data[n:]
    return train_data, val_data


main()
