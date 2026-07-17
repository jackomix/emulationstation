import re

with open('make_output.txt', 'r') as f:
    text = f.read()

undef_symbols = re.findall(r'"(.*?)", referenced from:', text)
print("\n".join(set(undef_symbols)))
