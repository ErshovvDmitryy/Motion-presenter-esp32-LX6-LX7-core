import tensorflow as tf
import numpy as np
import os
import pandas as pd
import random

print("🔄 Loading model...")
model = tf.keras.models.load_model('gesture_model.h5')

# Загружаем параметры нормализации
norm_params = np.load('norm_params.npz')
mean = norm_params['mean']
std = norm_params['std']

print("🔄 Preparing int8 quantization...")


def representative_dataset():
    dataset_path = "TEST_BIG_PACK_50_20"
    all_files = []

    # Собираем ВСЕ файлы
    for class_name in os.listdir(dataset_path):
        class_path = os.path.join(dataset_path, class_name)
        if not os.path.isdir(class_path):
            continue
        for file_name in os.listdir(class_path):
            if file_name.endswith('.csv'):
                all_files.append(os.path.join(class_path, file_name))

    # Берём 30-50% файлов для калибровки
    sample_size = min(500, int(len(all_files) * 0.3))
    selected_files = random.sample(all_files, sample_size)

    print(f"📊 Using {len(selected_files)} files for calibration")

    for file_path in selected_files:
        df = pd.read_csv(file_path)
        data = df.values.astype(np.float32)

        # ✅ ПРИМЕНЯЕМ НОРМАЛИЗАЦИЮ!
        data_norm = (data - mean) / (std + 1e-8)

        # Проверяем размер окна
        if data_norm.shape[0] != 49:
            continue

        yield [data_norm.reshape(1, 49, 6)]


# Конвертация
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.int8]
converter.representative_dataset = representative_dataset

print("🔄 Converting to TFLite int8...")
tflite_model = converter.convert()

# Сохраняем
output_path = 'gesture_model_int8.tflite'
with open(output_path, 'wb') as f:
    f.write(tflite_model)

print(f"✅ TFLite int8 model saved: {output_path}")
print(f"   Size: {len(tflite_model) / 1024:.2f} KB")