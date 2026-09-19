import os

print("🔄 Converting gesture_model_int8.tflite to C++ header...")

# Читаем TFLite модель
with open('gesture_model_int8.tflite', 'rb') as f:
    data = f.read()

# Создаём .h файл
with open('gesture_model_data.h', 'w') as f:
    f.write("#pragma once\n\n")
    f.write("#include <stdint.h>\n\n")

    f.write("const uint8_t gesture_model_data[] = {\n")

    # Записываем байты по 12 в строке
    for i, byte in enumerate(data):
        if i % 12 == 0:
            f.write("  ")
        f.write(f"0x{byte:02x}, ")
        if i % 12 == 11:
            f.write("\n")

    # Если последняя строка неполная — закрываем её
    if len(data) % 12 != 0:
        f.write("\n")

    f.write("};\n\n")
    f.write(f"const unsigned int gesture_model_len = {len(data)};\n")

print(f"✅ Created gesture_model_data.h")
print(f"   Size: {os.path.getsize('gesture_model_data.h') / 1024:.2f} KB")
print(f"   Model size: {len(data)} bytes")