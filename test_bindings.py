import scopemux_core

# Initialize the parser
parser = scopemux_core.ParserContext()

# Parse a simple C file
test_code = """
int add(int a, int b) {
    return a + b;
}
"""

with open("test.c", "w") as f:
    f.write(test_code)

# Parse the file
parser.parse_c_file_to_cst("test.c")

print("Successfully parsed C file")

# Get functions
functions = parser.get_nodes_by_type("function")
print(f"Found {len(functions)} functions")
