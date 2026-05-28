#!/usr/bin/env python3
"""
сборщик прошивки для dvd-процессора
использование: python3 asm.py firmware.asm firmware.bin

пример:
    mov  r0, 0x1000
    add  r1, r2
    jmp  loop
    mul  r3, r4
    push r5
    pop  r6
    ldrb r7, 0x2000
    strb 0x2004, r8
    swi  1
"""

import sys
import struct
import re

# таблица опкодов (полностью соответствует процессору)
OPCODES = {
    'nop':        0x00,
    'mov':        0x01,
    'add':        0x02,
    'load_frame': 0x03,
    'sub':        0x04,
    'and':        0x05,
    'or':         0x06,
    'xor':        0x07,
    'cmp':        0x08,
    'jmp':        0x09,
    'jz':         0x0a,
    'jnz':        0x0b,
    'load':       0x0c,
    'store':      0x0d,
    'halt':       0x0e,
    'inc':        0x0f,
    'dec':        0x10,
    'shl':        0x11,
    'shr':        0x12,
    'not':        0x13,
    'neg':        0x14,
    'call':       0x15,
    'ret':        0x16,
    'print_char': 0x17,
    'mul':        0x18,
    'div':        0x19,
    'mod':        0x1a,
    'lsl':        0x1b,
    'lsr':        0x1c,
    'asr':        0x1d,
    'ror':        0x1e,
    'push':       0x1f,
    'pop':        0x20,
    'movi':       0x21,
    'ldrb':       0x22,
    'strb':       0x23,
    'ldrh':       0x24,
    'strh':       0x25,
    'swi':        0x26,
}

def parse_reg(s):
    """r0..r15 -> число 0..15"""
    s = s.strip().lower()
    if s.startswith('r'):
        num = int(s[1:])
        if 0 <= num <= 15:
            return num
    raise ValueError(f"неправильный регистр: {s} (нужно r0..r15)")

def parse_value(s):
    """число в десятичном или hex виде (0x...)"""
    s = s.strip()
    if s.lower().startswith('0x'):
        return int(s, 16)
    return int(s)

def parse_byte(s):
    """число от 0 до 255"""
    val = parse_value(s)
    if 0 <= val <= 0xff:
        return val
    raise ValueError(f"значение {val} выходит за пределы 0..255")

def encode_imm32(val):
    return struct.pack('<I', val)

# ------------------- первый проход: метки и проверка аргументов -------------------
def first_pass(lines):
    labels = {}
    pc = 0
    for line in lines:
        if ';' in line:
            line = line[:line.index(';')]
        line = line.strip()
        if not line:
            continue

        # метка
        if ':' in line:
            label_part, rest = line.split(':', 1)
            label = label_part.strip().lower()
            if label:
                if label in labels:
                    raise ValueError(f"повторная метка: {label}")
                labels[label] = pc
            line = rest.strip()
            if not line:
                continue

        tokens = re.split(r'[,\s]+', line)
        if not tokens:
            continue
        mnemonic = tokens[0].lower()
        if mnemonic not in OPCODES:
            raise ValueError(f"неизвестная инструкция '{mnemonic}' по адресу {pc:#x}")

        # проверка числа аргументов (для удобства)
        argc = len(tokens) - 1
        if mnemonic in ('mov', 'load', 'ldrb', 'ldrh'):
            if argc != 2:
                raise ValueError(f"{mnemonic} нужно два аргумента: регистр, значение")
        elif mnemonic in ('store', 'strb', 'strh'):
            if argc != 2:
                raise ValueError(f"{mnemonic} нужно два аргумента: значение, регистр")
        elif mnemonic in ('add','sub','and','or','xor','cmp','shl','shr',
                          'mul','div','mod','lsl','lsr','asr','ror'):
            if argc != 2:
                raise ValueError(f"{mnemonic} нужно два регистра")
        elif mnemonic in ('jmp','jz','jnz','call'):
            if argc != 1:
                raise ValueError(f"{mnemonic} нужен адрес или метка")
        elif mnemonic in ('inc','dec','not','neg','print_char','push','pop'):
            if argc != 1:
                raise ValueError(f"{mnemonic} нужен один регистр")
        elif mnemonic == 'movi':
            if argc != 2:
                raise ValueError("movi нужен регистр и 16-битное значение")
        elif mnemonic == 'swi':
            if argc != 1:
                raise ValueError("swi нужен 8-битный код")
        elif mnemonic in ('nop','load_frame','ret','halt'):
            if argc != 0:
                raise ValueError(f"{mnemonic} не требует аргументов")

        # вычисление длины в байтах
        length = 1  # опкод
        if mnemonic == 'mov':
            length += 1 + 4
        elif mnemonic in ('add','sub','and','or','xor','cmp','shl','shr',
                          'mul','div','mod','lsl','lsr','asr','ror'):
            length += 2
        elif mnemonic in ('jmp','jz','jnz','call'):
            length += 4
        elif mnemonic == 'load':
            length += 1 + 4
        elif mnemonic == 'store':
            length += 4 + 1
        elif mnemonic in ('inc','dec','not','neg','print_char','push','pop'):
            length += 1
        elif mnemonic == 'movi':
            length += 1 + 2
        elif mnemonic in ('ldrb','ldrh','strb','strh'):
            length += 1 + 4
        elif mnemonic == 'swi':
            length += 1
        elif mnemonic in ('nop','load_frame','ret','halt'):
            length += 0
        else:
            raise ValueError(f"не знаю, сколько байт у {mnemonic}")

        pc += length
    return labels, pc

# ------------------- второй проход: генерация бинарного кода -------------------
def second_pass(lines, labels):
    pc = 0
    output = bytearray()
    for line in lines:
        if ';' in line:
            line = line[:line.index(';')]
        line = line.strip()
        if not line:
            continue

        # пропускаем метку, если она одна на строке
        if ':' in line:
            _, rest = line.split(':', 1)
            rest = rest.strip()
            if not rest:
                continue
            line = rest

        tokens = re.split(r'[,\s]+', line)
        if not tokens:
            continue
        mnemonic = tokens[0].lower()
        opcode = OPCODES[mnemonic]
        output.append(opcode)
        pc += 1

        # mov rX, imm32
        if mnemonic == 'mov':
            reg = parse_reg(tokens[1])
            imm = parse_value(tokens[2])
            output.append(reg)
            output.extend(encode_imm32(imm))
            pc += 5

        # двухрегистровые alu-операции
        elif mnemonic in ('add','sub','and','or','xor','cmp','shl','shr',
                          'mul','div','mod','lsl','lsr','asr','ror'):
            rd = parse_reg(tokens[1])
            rs = parse_reg(tokens[2])
            output.append(rd)
            output.append(rs)
            pc += 2

        # переходы и вызовы
        elif mnemonic in ('jmp','jz','jnz','call'):
            addr_str = tokens[1]
            if re.match(r'^0x[0-9a-f]+$', addr_str) or addr_str.isdigit():
                addr = parse_value(addr_str)
            else:
                if addr_str not in labels:
                    raise ValueError(f"метка '{addr_str}' не найдена (адрес {pc:#x})")
                addr = labels[addr_str]
            output.extend(encode_imm32(addr))
            pc += 4

        # load rX, addr32
        elif mnemonic == 'load':
            reg = parse_reg(tokens[1])
            addr = parse_value(tokens[2])
            output.append(reg)
            output.extend(encode_imm32(addr))
            pc += 5

        # store addr32, rX
        elif mnemonic == 'store':
            addr = parse_value(tokens[1])
            reg = parse_reg(tokens[2])
            output.extend(encode_imm32(addr))
            output.append(reg)
            pc += 5

        # инструкции с одним регистром
        elif mnemonic in ('inc','dec','not','neg','print_char','push','pop'):
            reg = parse_reg(tokens[1])
            output.append(reg)
            pc += 1

        # movi rX, imm16 (little-endian)
        elif mnemonic == 'movi':
            reg = parse_reg(tokens[1])
            imm = parse_value(tokens[2]) & 0xffff
            output.append(reg)
            output.append(imm & 0xff)         # младший байт
            output.append((imm >> 8) & 0xff)  # старший байт
            pc += 3

        # ldrb rX, addr32
        elif mnemonic == 'ldrb':
            reg = parse_reg(tokens[1])
            addr = parse_value(tokens[2])
            output.append(reg)
            output.extend(encode_imm32(addr))
            pc += 5

        # strb addr32, rX
        elif mnemonic == 'strb':
            addr = parse_value(tokens[1])
            reg = parse_reg(tokens[2])
            output.extend(encode_imm32(addr))
            output.append(reg)
            pc += 5

        # ldrh rX, addr32
        elif mnemonic == 'ldrh':
            reg = parse_reg(tokens[1])
            addr = parse_value(tokens[2])
            output.append(reg)
            output.extend(encode_imm32(addr))
            pc += 5

        # strh addr32, rX
        elif mnemonic == 'strh':
            addr = parse_value(tokens[1])
            reg = parse_reg(tokens[2])
            output.extend(encode_imm32(addr))
            output.append(reg)
            pc += 5

        # swi imm8
        elif mnemonic == 'swi':
            code = parse_byte(tokens[1])
            output.append(code)
            pc += 1

        # без операндов
        elif mnemonic in ('nop','load_frame','ret','halt'):
            pass

        else:
            raise ValueError(f"необработанная инструкция: {mnemonic}")

    return bytes(output)

# ------------------- точка входа -------------------
def main():
    if len(sys.argv) != 3:
        print(f"использование: {sys.argv[0]} <файл.asm> <выходной.bin>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"ошибка при чтении {input_file}: {e}")
        sys.exit(1)

    try:
        labels, _ = first_pass(lines)
        binary = second_pass(lines, labels)
    except Exception as e:
        print(f"ошибка сборки: {e}")
        sys.exit(1)

    try:
        with open(output_file, 'wb') as f:
            f.write(binary)
    except Exception as e:
        print(f"ошибка при записи {output_file}: {e}")
        sys.exit(1)

    print(f"успешно! собрано {len(binary)} байт -> {output_file}")

if __name__ == '__main__':
    main()
