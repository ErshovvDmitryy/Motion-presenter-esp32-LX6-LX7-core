import numpy as np
import pandas as pd
import os

dataset_path = "TEST_BIG_PACK_50_20"

# Загружаем все данные
X = []
for class_name in os.listdir(dataset_path):
    class_path = os.path.join(dataset_path, class_name)
    if not os.path.isdir(class_path):
        continue
    for file_name in os.listdir(class_path):
        if file_name.endswith('.csv'):
            df = pd.read_csv(os.path.join(class_path, file_name))
            X.append(df.values)

X = np.array(X)  # (N, 49, 6)

# Считаем Z-Score
X_flat = X.reshape(-1, 6)
mean = np.mean(X_flat, axis=0)
std = np.std(X_flat, axis=0)

# Смотрим, какие значения после Z-Score
X_norm = (X - mean) / (std + 1e-8)

print("📊 Анализ после Z-Score:")
print("-" * 60)
for i in range(6):
    values = X_norm[:, :, i].flatten()
    print(f"Признак {i}:")
    print(f"  Min: {values.min():.2f}")
    print(f"  Max: {values.max():.2f}")
    print(f"  Std: {values.std():.2f}")
    print(f"  99.9% перцентиль: {np.percentile(np.abs(values), 99.9):.2f}")
    print()

# Оцениваем потерю точности при int8
print("🔍 Оценка для int8:")
print("-" * 60)
for i in range(6):
    max_val = np.percentile(np.abs(X_norm[:, :, i].flatten()), 99.9)
    # int8: 255 уровней на диапазон [-max_val, max_val]
    levels_per_unit = 255 / (2 * max_val + 1e-8)
    print(f"Признак {i}: {levels_per_unit:.1f} уровней на единицу")

    # Если < 10, то точность плохая
    if levels_per_unit < 10:
        print(f"  ⚠️ ВНИМАНИЕ! Меньше 10 уровней — возможна потеря точности")
    else:
        print(f"  ✅ OK")