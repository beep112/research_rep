import os

from graph_utils import Graph


def main():

    print("\n=== Test 1: Creating graph ===")
    g1 = Graph()

    # adding nodes
    g1.add_node("A", label="Start", shape="box")
    g1.add_node("B", label="Process", shape="ellipse")
    g1.add_node("C", label="Decision", shape="diamond")
    g1.add_node("D", label="End", shape="box")

    # adding edges
    g1.add_edge("A", "B", label="next")
    g1.add_edge("B", "C", label="evaluate")
    g1.add_edge("C", "D", label="yes")
    g1.add_edge("C", "B", label="no", style="dashed")

    # save to .dot file
    g1.save_to_dot("test1_graph.dot")

    # display the graph
    print("Displaying graph with GraphViz (dot layout):")
    g1.display_with_graphviz(layout="dot")

    print("\nDisplaying graph with Matplotlib:")
    g1.display_with_matplotlib()

    print("\n=== Test 2: Loading graph from .dot file ===")
    if os.path.exists("example_graph.dot"):
        g2 = Graph()
        if g2.load_from_dot("example_graph.dot"):
            print(f"Number of nodes: {g2.get_node_count()}")
            print(f"Number of edges: {g2.get_edge_count()}")
            print(f"Nodes: {g2.get_nodes()}")
            print(f"Edges: {g2.get_edges()}")

            # Display with a different layout
            print("\nDisplaying graph with GraphViz (neato layout):")
            g2.display_with_graphviz(layout="neato")

    print("\n=== Test 3: Creating a more complex graph ===")
    g3 = Graph()

    # Add nodes
    for i in range(1, 11):
        g3.add_node(f"Node{i}")

    # Add edges to create a circular structure with some cross connections
    for i in range(1, 10):
        g3.add_edge(f"Node{i}", f"Node{i+1}")
    g3.add_edge("Node10", "Node1")  # Complete the circle

    # Add some cross connections
    g3.add_edge("Node1", "Node5")
    g3.add_edge("Node3", "Node8")
    g3.add_edge("Node7", "Node2")

    # Display the graph with different layouts
    print("\nDisplaying complex graph with GraphViz (dot layout):")
    g3.display_with_graphviz(layout="dot")

    print("\nDisplaying complex graph with GraphViz (circo layout):")
    g3.display_with_graphviz(layout="circo")

    print("\nDisplaying complex graph with GraphViz (fdp layout):")
    g3.display_with_graphviz(layout="fdp")

    print("\nDisplaying complex graph with Matplotlib:")
    g3.display_with_matplotlib()


if __name__ == "__main__":
    main()
