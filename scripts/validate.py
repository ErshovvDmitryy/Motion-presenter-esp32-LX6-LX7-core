import tensorflow as tf
import numpy as np
import os
import pandas as pd
from sklearn.model_selection import train_test_split


# ============================================================================
# 1. ЗАГРУЗКА ДАННЫХ (как в основном скрипте)
# ============================================================================

def load_test_data(dataset_path):
    """Загружает данные для тестирования"""
    X = []
    y = []
    class_names = []

    for class_name in os.listdir(dataset_path):
        class_path = os.path.join(dataset_path, class_name)
        if not os.path.isdir(class_path):
            continue

        class_names.append(class_name)

        for file_name in os.listdir(class_path):
            if not file_name.endswith('.csv'):
                continue

            file_path = os.path.join(class_path, file_name)
            df = pd.read_csv(file_path)
            data = df.values
            X.append(data)
            y.append(class_name)

    X = np.array(X, dtype=np.float32)
    class_to_idx = {name: i for i, name in enumerate(class_names)}
    y = np.array([class_to_idx[label] for label in y])
    y = tf.keras.utils.to_categorical(y, num_classes=len(class_names))

    return X, y, class_names, class_to_idx


# ============================================================================
# 2. ЗАГРУЗКА ПАРАМЕТРОВ НОРМАЛИЗАЦИИ
# ============================================================================

# Загружаем параметры нормализации
norm_params = np.load('norm_params.npz')
mean = norm_params['mean']
std = norm_params['std']

# Загружаем классы
with open('class_names.txt', 'r') as f:
    class_names = [line.strip() for line in f.readlines()]

# ============================================================================
# 3. ЗАГРУЗКА ТЕСТОВЫХ ДАННЫХ
# ============================================================================

dataset_path = "TEST_BIG_PACK_50_20"
X, y, _, _ = load_test_data(dataset_path)

# Разделяем на train/val/test (используем тот же random_state)
X_train, X_temp, y_train, y_temp = train_test_split(
    X, y, test_size=0.3, random_state=42, stratify=y
)
X_val, X_test, y_val, y_test = train_test_split(
    X_temp, y_temp, test_size=0.5, random_state=42, stratify=y_temp
)

# Нормализуем тестовые данные
X_test_norm = (X_test - mean) / (std + 1e-8)

print(f"📊 Test samples: {len(X_test_norm)}")
print(f"   Shape: {X_test_norm.shape}")

# ============================================================================
# 4. ПРОВЕРКА FLOAT32 МОДЕЛИ
# ============================================================================

print("\n" + "=" * 60)
print("🔍 Проверка float32 модели")
print("=" * 60)

model_float = tf.keras.models.load_model('gesture_model.h5')
loss_float, acc_float = model_float.evaluate(X_test_norm, y_test, verbose=0)
print(f"✅ Float32 Test Accuracy: {acc_float:.4f} ({acc_float * 100:.2f}%)")

# ============================================================================
# 5. ПРОВЕРКА INT8 МОДЕЛИ (TFLite)
# ============================================================================

print("\n" + "=" * 60)
print("🔍 Проверка int8 модели (TFLite)")
print("=" * 60)

# Загружаем TFLite модель
interpreter = tf.lite.Interpreter(model_path='gesture_model_int8.tflite')
interpreter.allocate_tensors()

# Получаем информацию о входах/выходах
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print(f"📐 Input shape: {input_details[0]['shape']}")
print(f"📐 Input dtype: {input_details[0]['dtype']}")
print(f"📐 Output shape: {output_details[0]['shape']}")
print(f"📐 Output dtype: {output_details[0]['dtype']}")


# Функция для инференса через TFLite
def predict_tflite(interpreter, X):
    """Предсказание для одного или нескольких образцов"""
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    predictions = []
    for i in range(len(X)):
        # Берем один образец
        sample = X[i:i + 1]  # (1, 49, 6)

        # Если модель ожидает int8, нужно преобразовать в int8
        if input_details[0]['dtype'] == np.int8:
            # Масштабируем в диапазон int8 [-128, 127]
            input_scale, input_zero_point = input_details[0]['quantization']
            sample_int8 = (sample / input_scale + input_zero_point).astype(np.int8)
            interpreter.set_tensor(input_details[0]['index'], sample_int8)
        else:
            # Если ожидает float32 (на случай, если квантизация была не полной)
            interpreter.set_tensor(input_details[0]['index'], sample.astype(np.float32))

        # Инференс
        interpreter.invoke()

        # Получаем результат
        output_data = interpreter.get_tensor(output_details[0]['index'])

        # Если выход в int8, преобразуем обратно в float
        if output_details[0]['dtype'] == np.int8:
            output_scale, output_zero_point = output_details[0]['quantization']
            output_data = (output_data.astype(np.float32) - output_zero_point) * output_scale

        predictions.append(output_data[0])

    return np.array(predictions)


# Предсказания
print("\n🔄 Выполняем инференс на TFLite...")
y_pred_tflite = predict_tflite(interpreter, X_test_norm)

# Преобразуем one-hot в классы
y_true_classes = np.argmax(y_test, axis=1)
y_pred_classes = np.argmax(y_pred_tflite, axis=1)

# Считаем точность
from sklearn.metrics import accuracy_score, classification_report

acc_tflite = accuracy_score(y_true_classes, y_pred_classes)
print(f"\n✅ Int8 TFLite Test Accuracy: {acc_tflite:.4f} ({acc_tflite * 100:.2f}%)")

# ============================================================================
# 6. СРАВНЕНИЕ РЕЗУЛЬТАТОВ
# ============================================================================

print("\n" + "=" * 60)
print("📊 Сравнение моделей")
print("=" * 60)

print(f"Float32 model:  {acc_float * 100:.2f}%")
print(f"Int8 model:     {acc_tflite * 100:.2f}%")
print(f"Разница:        {(acc_float - acc_tflite) * 100:.2f}%")
print(f"Размер float32: {os.path.getsize('gesture_model.h5') / 1024:.2f} KB")
print(f"Размер int8:    {os.path.getsize('gesture_model_int8.tflite') / 1024:.2f} KB")

# ============================================================================
# 7. ДЕТАЛЬНЫЙ ОТЧЕТ ПО КЛАССАМ (для int8)
# ============================================================================

print("\n" + "=" * 60)
print("📋 Детальный отчет по классам (int8)")
print("=" * 60)

print(classification_report(
    y_true_classes,
    y_pred_classes,
    target_names=class_names,
    digits=4
))

# ============================================================================
# 8. СОХРАНЯЕМ РЕЗУЛЬТАТЫ
# ============================================================================

with open('model_comparison.txt', 'w') as f:
    f.write("=" * 60 + "\n")
    f.write("СРАВНЕНИЕ МОДЕЛЕЙ\n")
    f.write("=" * 60 + "\n\n")
    f.write(f"Float32 accuracy: {acc_float * 100:.2f}%\n")
    f.write(f"Int8 accuracy:    {acc_tflite * 100:.2f}%\n")
    f.write(f"Разница:          {(acc_float - acc_tflite) * 100:.2f}%\n")
    f.write(f"Размер float32:   {os.path.getsize('gesture_model.h5') / 1024:.2f} KB\n")
    f.write(f"Размер int8:      {os.path.getsize('gesture_model_int8.tflite') / 1024:.2f} KB\n")

print("\n✅ Результаты сохранены в 'model_comparison.txt'")
print("=" * 60)