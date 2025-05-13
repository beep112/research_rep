import os
import sys

from graph_utils import Graph


def load_and_display_dot_file(dot_file_path):
    """
    @brief Load a .dot file and display it using both GraphViz and Matplotlib.
    @param dot_file_path: Path to the .dot file
    """
    # Create output directory if it doesn't exist
    os.makedirs("graph_output", exist_ok=True)

    # Create a new Graph instance
    g = Graph()

    # Get the base filename without extension
    base_name = os.path.splitext(os.path.basename(dot_file_path))[0]

    # Load the .dot file
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
        print(f"Failed to load {dot_file_path}")


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
