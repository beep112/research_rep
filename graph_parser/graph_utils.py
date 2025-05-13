import os
import tempfile
import time

import matplotlib.pyplot as plt
import networkx as nx
import pydot
from IPython.display import Image, display


class Graph:
    """
    @brief Attempting to create a custom Graph class that can load, manipulate, and visualize graphs from .dot files
    these are basic graphs right now
    """

    def __init__(self):
        """
        @brief Initalize an empty graph

        @param self: the current graph object
        """
        self.graph = nx.DiGraph()

    def load_from_dot(self, file_path):
        """
        @brief Loads a graph from a .dot file.

        @param file_path: Path to the .dot file

        @return bool: True if successful, False if fails for any reason
        """
        try:
            # function used to read from a dot file
            graphs = pydot.graph_from_dot_file(file_path)
            if not graphs:
                print("No graphs was found in the .dot file (check the file maybe?)")
                return False
            # Conversion to a netwrokx graph
            dot_graph = graphs[0]
            if dot_graph.get_type() == "graph":
                self.graph = nx.Graph(nx.drawing.nx_pydot.from_pydot(dot_graph))
            else:
                self.graph = nx.DiGraph(nx.drawing.nx_pydot.from_pydot(dot_graph))
            print(
                f"Success: Graph with {len(list(self.graph.nodes()))} nodes and {len(self.graph.edges())} edges"
            )
            return True

        except Exception as e:
            print(f"Error loading .dot file: {e}")
            return False

    def add_node(self, node_id, **attributes):
        """
        @brief Add a node to the current graph

        @param self: the current graph object
        @param node_id: tage for the node
        @param **attributes: optional node features
        """
        self.graph.add_node(node_id, **attributes)

    def add_edge(self, start, dest, **attributes):
        """
        @brief Add an edge to the graph

        @param self: the current graph object
        @param start: The node to start at
        @param dest: the node to end at
        @param **attributes: optional edge features
        """
        self.graph.add_edge(start, dest, **attributes)

    def save_to_dot(self, file_path):
        """
        @brief Save the current graph to a .dot file

        @param self: the current graph object
        @param file_path: output file path
        @return bool: True if success, false if something failed
        """
        try:
            # conversion from Networkx to dot
            pydot_graph = nx.drawing.nx_pydot.to_pydot(self.graph)

            pydot_graph.write_dot(file_path)
            print(f"Graph saved to {file_path}")
            return True
        except Exception as e:
            print(f"Error saving graph to .dot file: {e}")
            return False

    def display_with_graphviz(
        self, layout="dot", format="png", show=True, output_file=None
    ):
        """
        @brief Visualize with GraphViz by creating a png

        @param layout: GraphViz layout type ('dot', 'neato', 'fdp', 'sfdp', 'twopi', 'circo')
        @param format: Output format ('png', 'svg', 'pdf', etc.)
        @param show: Display the graph (if true) or return the image data (false)
        @return bytes or None: Image data if show=False, otherwise None
        """
        try:
            # Convert NetworkX graph to pydot
            pydot_graph = nx.drawing.nx_pydot.to_pydot(self.graph)

            # Set the layout engine
            pydot_graph.set_layout(layout)

            # If output file is provided, save directly to it
            if output_file:
                pydot_graph.write(output_file, format=format)
                print(f"Graph visualization saved to {output_file}")
                return None

            # Otherwise, proceed with temporary file approach for display or return
            with tempfile.NamedTemporaryFile(suffix=f".{format}", delete=False) as tmp:
                temp_name = tmp.name

            # Render the graph to the temporary file
            pydot_graph.write(temp_name, format=format)

            if show:
                try:
                    # Try to display the image (works in Jupyter/IPython)
                    img = Image(filename=temp_name)
                    display(img)
                except (NameError, ImportError):
                    # If we're in a terminal, just inform the user where the file is
                    print(f"Graph saved to temporary file: {temp_name}")
                    print(
                        "(Note: Interactive display not available in terminal environment)"
                    )
                return None
            else:
                # Return the image data
                with open(temp_name, "rb") as f:
                    img_data = f.read()
                os.unlink(temp_name)  # Clean up
                return img_data

        except Exception as e:
            print(f"Error displaying graph: {e}")
            return None

    def display_with_matplotlib(
        self,
        layout=None,
        node_size=300,
        node_color="skyblue",
        edge_color="black",
        with_labels=True,
        font_size=10,
        figsize=(8, 6),
        output_file=None,
    ):
        """
        @brief Trying and seeing how visualization of graphs with Matplotlib looks like.

        @param layout: the layout algorithm used
        @param node_size: size of nodes
        @param node_color: color of nodes
        @param edge_color: color of edges
        @param with_labels: whether to display node labels
        @param font_size: size of node labels
        @param figsize: figure size
        """
        try:
            # Create a new figure with the specified size
            fig = plt.figure(figsize=figsize)

            # Generate layout positions
            if layout is None:
                # Choose layout based on graph type
                if nx.is_directed(self.graph):
                    pos = nx.spring_layout(self.graph)
                else:
                    # Fallback to spring_layout if kamada_kawai fails (which can happen with certain graph structures)
                    try:
                        pos = nx.kamada_kawai_layout(self.graph)
                    except:
                        pos = nx.spring_layout(self.graph)
            else:
                # Use the provided layout function
                pos = layout(self.graph)

            # Draw the graph
            nx.draw_networkx(
                self.graph,
                pos=pos,
                with_labels=with_labels,
                node_size=node_size,
                node_color=node_color,
                edge_color=edge_color,
                font_size=font_size,
            )

            # Remove axis
            plt.gca().set_axis_off()

            # Adjust layout
            plt.tight_layout()

            # Save to file if requested
            if output_file:
                plt.savefig(output_file)
                plt.close()
                print(f"Matplotlib visualization saved to {output_file}")
            else:
                # Try to show interactively (may not work in terminal)
                try:
                    plt.show()
                except Exception as e:
                    # Generate a temporary file if showing fails
                    temp_file = f"graph_matplotlib_{int(time.time())}.png"
                    plt.savefig(temp_file)
                    plt.close()
                    print(f"Matplotlib visualization saved to {temp_file}")
                    print(
                        "(Note: Interactive display not available in terminal environment)"
                    )

        except Exception as e:
            print(f"Error displaying graph with matplotlib: {e}")
            import traceback

            traceback.print_exc()

    def get_node_count(self):
        """
        @brief return the number of nodes in the graph.
        @return num of nodes in graph
        """
        return len(list(self.graph.nodes()))

    def get_edge_count(self):
        """
        @brief return the number of edges in the graph.
        @return num of edges in graph
        """
        return len(self.graph.edges())

    def get_nodes(self):
        """
        @brief return the list of nodes in the graph
        @return list of nodes in graph
        """
        return list(self.graph.nodes())

    def get_edges(self):
        """
        @brief return the list of edges in the graph
        @return list of edges in graph
        """
        return list(self.graph.edges())

    def get_successors(self, node):
        """
        @brief return successors of a node
        @param node: the node which will be check for successor
        @return list of successors from node
        """
        return list(self.graph.successors(node))

    def get_predecessors(self, node):
        """
        @brief reutrn the predessors of a node
        @param node: the node which predecessors will be checked from
        @return list of predecessors from node
        """
        return list(self.graph.predecessors(node))
