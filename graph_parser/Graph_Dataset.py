import os

import numpy as np

"""
@file Graph_Dataset.py
@brief a way to store and parse multiple graphs store in a file

The way that graphs will be stores is 
Graph
[
    node1 node2 edge_cost
    node3 node4 edge_cost
    node5 (can also access single nodes)
]

@author Jason Perndreca
@note WORK IN PROGRESS (INCOMPLETE)
@version %I%, %G%
"""


"""
@brief class used to parse data for graphs and store them in a list


@note The data represenation will work on singular nodes
"""


class GraphDataset:
    def __init__(self, file_path):
        """
        @brief constructor for GraphDataset object

        @param file_path (string): file path where graph data is stored
        @throws ParseError: If filepath is not valid or format is invalid
        """
        self.file_path = file_path
        self.graphs = []
        self.load_graphs()

    def load_graphs(self):
        """Load all graphs from the data file into an adjacency list representation."""
        try:
            with open(self.file_path, "r") as f:
                content = f.read()

            # Split the content by 'Graph ['
            graph_blocks = content.split("Graph [")

            # Skip the first element if it's empty
            if not graph_blocks[0].strip():
                graph_blocks = graph_blocks[1:]

            for block in graph_blocks:
                # Remove trailing ']' and split by lines
                block = block.strip()
                if block.endswith("]"):
                    block = block[:-1]

                lines = [line.strip() for line in block.split("\n") if line.strip()]

                # Create a new graph as an adjacency list
                graph = {}

                for line in lines:
                    parts = line.split()

                    # Handle singleton node (just one node identifier on the line)
                    if len(parts) == 1:
                        node = parts[0]
                        if node not in graph:
                            graph[node] = []
                    # Handle regular edge definition
                    elif len(parts) >= 3:
                        node1, node2, edge_cost = parts[0], parts[1], float(parts[2])

                        # Add nodes if they don't exist
                        if node1 not in graph:
                            graph[node1] = []
                        if node2 not in graph:
                            graph[node2] = []

                        # Add edge with cost
                        graph[node1].append((node2, edge_cost))

                if graph:  # Only add non-empty graphs
                    self.graphs.append(graph)

        except Exception as e:
            print(f"Error loading graphs: {e}")
