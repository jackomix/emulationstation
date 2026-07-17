import re

with open('make_output.txt', 'r') as f:
    text = f.read()

symbols = re.findall(r'"([^"]+)", referenced from:', text)
symbols = set(symbols)

for sym in sorted(symbols):
    sym = sym.replace('std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>', 'std::string')
    sym = sym.replace('std::__1::vector<HelpPrompt, std::__1::allocator<HelpPrompt>>', 'std::vector<HelpPrompt>')
    sym = sym.replace('std::__1::function', 'std::function')
    sym = sym.replace('std::__1::shared_ptr', 'std::shared_ptr')
    sym = sym.replace('std::__1::map', 'std::map')
    sym = sym.replace('std::__1::pair', 'std::pair')
    sym = sym.replace('std::__1::less', 'std::less')
    sym = sym.replace('std::__1::allocator', 'std::allocator')
    sym = re.sub(r'const&', 'const&', sym)
    
    if sym.startswith('typeinfo for '): continue
    if sym.startswith('vtable for '): continue
    
    print(sym)

