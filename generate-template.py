import os

full = ""
ignored_extensions = ('.o', '.elf', '.iso')

# Loop through all files in the 'src' directory
for f in os.listdir("src"):
    # Construct the full path to the file
    filepath = os.path.join("src", f)
    
    # Check if the current item is a file and does not have an ignored extension
    if os.path.isfile(filepath) and not f.endswith(ignored_extensions):
        # Add the filename as a header
        full += f + ":\n\n"
        
        # Open the file, specifying an encoding and an error handling strategy.
        # 'utf-8' is tried first. If an invalid byte is found, it will be
        # replaced with a placeholder character '' instead of crashing.
        # You could also use encoding='latin-1' which is less strict.
        try:
            with open(filepath, "r", encoding='utf-8', errors='replace') as ff:
                full += ff.read() + "\n\n"
        except IOError as e:
            print(f"Could not read file {filepath}: {e}")


# Write the combined content to the output file, ensuring it's UTF-8 encoded.
try:
    with open("template.cpp", "w", encoding='utf-8') as f:
        f.write(full)
except IOError as e:
    print(f"Could not write to template.cpp: {e}")

print("Successfully created template.cpp")

