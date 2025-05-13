import os
import sys

import matplotlib.pyplot as plt
import networkx as nx
import pydot
from graph_utils import Graph


def direct_load_dot(dot_file_path):
    """
    @brief Load a DOT file directly with pydot, bypassing NetworkX's parser.

    @param    dot_file_path: Path to the DOT file

    @return    A NetworkX DiGraph object
    """
    try:
        print("Attempting to load DOT file directly with pydot...")
        graphs = pydot.graph_from_dot_file(dot_file_path)

        if not graphs:
            print("No graphs found in the DOT file.")
            return None

        dot_graph = graphs[0]
        print(f"DOT graph loaded successfully. Type: {dot_graph.get_type()}")

        # Convert to NetworkX graph
        if dot_graph.get_type() == "graph":
            graph = nx.Graph()
        else:
            graph = nx.DiGraph()

        # Manual conversion without using nx_pydot
        # Add nodes
        for node in dot_graph.get_nodes():
            node_name = node.get_name().strip('"')
            attrs = {
                k.strip('"'): v.strip('"') for k, v in node.get_attributes().items()
            }
            graph.add_node(node_name, **attrs)

        # Add edges
        for edge in dot_graph.get_edges():
            source = edge.get_source().strip('"')
            target = edge.get_destination().strip('"')
            attrs = {
                k.strip('"'): v.strip('"') for k, v in edge.get_attributes().items()
            }
            graph.add_edge(source, target, **attrs)

        print(
            f"Converted to NetworkX graph with {graph.number_of_nodes()} nodes and {graph.number_of_edges()} edges"
        )
        return graph

    except Exception as e:
        print(f"Error in direct_load_dot: {e}")
        import traceback

        traceback.print_exc()
        return None


def load_and_display_dot_file(dot_file_path):
    """
    Load a .dot file and display it using both GraphViz and Matplotlib.

    Args:
        dot_file_path: Path to the .dot file
    """
    # Run debug to check package versions

    # Create output directory if it doesn't exist
    os.makedirs("graph_output", exist_ok=True)

    # Get the base filename without extension
    base_name = os.path.splitext(os.path.basename(dot_file_path))[0]

    # Try to load the graph directly with pydot first
    graph = direct_load_dot(dot_file_path)

    if graph:
        print(f"\nGraph Information:")
        print(f"Nodes: {graph.number_of_nodes()}")
        print(f"Edges: {graph.number_of_edges()}")

        # Save visualizations
        print("\nSaving visualizations to files:")

        try:
            # Try to save with pydot directly first
            print("Saving with pydot...")
            pydot_graph = nx.drawing.nx_pydot.to_pydot(graph)

            # GraphViz with dot layout
            output_file = f"graph_output/{base_name}_dot.png"
            pydot_graph.write_png(output_file, prog="dot")
            print(f"GraphViz dot visualization saved to {output_file}")

            # GraphViz with neato layout
            output_file = f"graph_output/{base_name}_neato.png"
            pydot_graph.write_png(output_file, prog="neato")
            print(f"GraphViz neato visualization saved to {output_file}")

        except Exception as e:
            print(f"Error saving with pydot: {e}")

        try:
            # Matplotlib
            print("Saving with matplotlib...")
            output_file = f"graph_output/{base_name}_matplotlib.png"
            plt.figure(figsize=(12, 10))
            pos = nx.spring_layout(graph, seed=42)
            nx.draw(
                graph,
                pos,
                with_labels=True,
                node_color="lightblue",
                node_size=1500,
                arrows=True if nx.is_directed(graph) else False,
                arrowsize=20,
                edge_color="gray",
            )
            plt.savefig(output_file)
            plt.close()
            print(f"Matplotlib visualization saved to {output_file}")

        except Exception as e:
            print(f"Error saving with matplotlib: {e}")

        print(f"\nAll available visualizations saved to 'graph_output' directory.")
    else:
        print(f"Failed to load {dot_file_path}")

        # Try with your original Graph class as a fallback
        print("\nTrying with original Graph class...")
        try:
            g = Graph()
            if g.load_from_dot(dot_file_path):
                print(f"\nGraph Information:")
                print(f"Nodes: {g.get_node_count()}")
                print(f"Edges: {g.get_edge_count()}")

                # Save visualizations with different layouts
                print("\nSaving visualizations to files:")

                # GraphViz with dot layout
                output_file = f"graph_output/{base_name}_dot.png"
                g.display_with_graphviz(layout="dot", output_file=output_file)

                # GraphViz with neato layout
                output_file = f"graph_output/{base_name}_neato.png"
                g.display_with_graphviz(layout="neato", output_file=output_file)

                # Matplotlib
                output_file = f"graph_output/{base_name}_matplotlib.png"
                g.display_with_matplotlib(output_file=output_file)

                print(f"\nAll visualizations saved to 'graph_output' directory.")
            else:
                print(f"Failed to load {dot_file_path} with Graph class")
        except Exception as e:
            print(f"Error with Graph class: {e}")
            import traceback

            traceback.print_exc()


if __name__ == "__main__":
    # Check if a file path was provided as an argument
    if len(sys.argv) > 1:
        dot_file_path = sys.argv[1]
        load_and_display_dot_file(dot_file_path)
    else:
        # Use the sample graph if no argument provided
        if os.path.exists("sample_graph.dot"):
            print("No .dot file specified. Using 'sample_graph.dot'.")
            load_and_display_dot_file("sample_graph.dot")
        else:
            print("No .dot file specified and sample_graph.dot doesn't exist.")
            print("Please provide a .dot file path as an argument.")
