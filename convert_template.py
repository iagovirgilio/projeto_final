#!/usr/bin/env python3
import sys

if len(sys.argv) != 3:
    print("Usage: {} input.html output.h".format(sys.argv[0]))
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(input_file, 'r', encoding='utf-8') as f:
    data = f.read()

# Escapa as barras invertidas e aspas, e converte as quebras de linha
data = data.replace('\\', '\\\\')
data = data.replace('"', '\\"')
data = data.replace('\n', '\\n"\n"')

output = '#ifndef TEMPLATE_H\n#define TEMPLATE_H\n\n'
output += 'const char html_template[] = \"' + data + '";\n\n'
output += '#endif // TEMPLATE_H\n'

with open(output_file, 'w', encoding='utf-8') as f:
    f.write(output)
