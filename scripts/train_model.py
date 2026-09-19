import os
import numpy as np
import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import matplotlib.pyplot as plt
from datetime import datetime


# ============================================================================
# 1. ЗАГРУЗКА ДАННЫХ
# ============================================================================

def load_dataset(dataset_path):
    """
    Загружает датасет из папок с CSV файлами.
    """
    X = []
    y = []
    class_names = []

    print(f"📂 Loading dataset from: {dataset_path}")

    # Получаем список классов (папок)
    for class_name in os.listdir(dataset_path):
        class_path = os.path.join(dataset_path, class_name)
        if not os.path.isdir(class_path):
            continue

        class_names.append(class_name)
        file_count = 0

        # Читаем все CSV файлы в папке
        for file_name in os.listdir(class_path):
            if not file_name.endswith('.csv'):
                continue

            file_path = os.path.join(class_path, file_name)
            df = pd.read_csv(file_path)

            # Преобразуем в numpy array
            data = df.values  # (80, 6)
            X.append(data)
            y.append(class_name)
            file_count += 1

        print(f"   {class_name}: {file_count} files")

    # Преобразуем в numpy arrays
    X = np.array(X, dtype=np.float32)

    # Преобразуем классы в числа
    class_to_idx = {name: i for i, name in enumerate(class_names)}
    y = np.array([class_to_idx[label] for label in y])

    # One-hot encoding
    y = tf.keras.utils.to_categorical(y, num_classes=len(class_names))

    print(f"\n✅ Total: {len(X)} samples")
    print(f"   Classes: {class_names}")
    print(f"   X shape: {X.shape}")
    print(f"   y shape: {y.shape}")

    return X, y, class_names, class_to_idx


# ============================================================================
# 2. СОЗДАНИЕ МОДЕЛИ
# ============================================================================

def create_model(window_size=49, num_features=6, num_classes=8):
    model = tf.keras.Sequential([
        # Вход
        tf.keras.layers.Input(shape=(window_size, num_features)),

        # Слой 1: 16 фильтров, ядро 5
        tf.keras.layers.Conv1D(16, 5, activation='relu', padding='same'),
        tf.keras.layers.MaxPooling1D(2),
        tf.keras.layers.Dropout(0.2),

        # Слой 2: 32 фильтра, ядро 5
        tf.keras.layers.Conv1D(32, 5, activation='relu', padding='same'),
        tf.keras.layers.MaxPooling1D(2),
        tf.keras.layers.Dropout(0.2),

        # Слой 3: 64 фильтра, ядро 3
        tf.keras.layers.Conv1D(64, 3, activation='relu', padding='same'),
        tf.keras.layers.GlobalAveragePooling1D(),
        tf.keras.layers.Dropout(0.3),

        # Dense слои (уменьшены)
        tf.keras.layers.Dense(32, activation='relu'),
        tf.keras.layers.Dropout(0.3),

        # Выход
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])

    model.compile(
        optimizer='adam',
        loss='categorical_crossentropy',
        metrics=['accuracy']
    )

    return model


# ============================================================================
# 3. ОБУЧЕНИЕ МОДЕЛИ
# ============================================================================

def train_model(dataset_path, epochs=50, batch_size=32):
    """
    Основная функция обучения
    """
    # 1. Загружаем данные
    X, y, class_names, class_to_idx = load_dataset(dataset_path)

    # 2. Разделяем на train/validation/test
    X_train, X_temp, y_train, y_temp = train_test_split(
        X, y, test_size=0.3, random_state=42, stratify=y
    )
    X_val, X_test, y_val, y_test = train_test_split(
        X_temp, y_temp, test_size=0.5, random_state=42, stratify=y_temp
    )

    print(f"\n📊 Data split:")
    print(f"   Train: {len(X_train)}")
    print(f"   Val:   {len(X_val)}")
    print(f"   Test:  {len(X_test)}")

    # 3. Нормализация (Z-Score)
    X_flat = X_train.reshape(-1, X_train.shape[2])
    mean = np.mean(X_flat, axis=0)
    std = np.std(X_flat, axis=0)

    X_train_norm = (X_train - mean) / (std + 1e-8)
    X_val_norm = (X_val - mean) / (std + 1e-8)
    X_test_norm = (X_test - mean) / (std + 1e-8)

    print(f"\n📐 Normalization params:")
    print(f"   Mean: {mean}")
    print(f"   Std:  {std}")

    # 4. Создаем модель
    model = create_model(
        window_size=X_train.shape[1],
        num_features=X_train.shape[2],
        num_classes=y_train.shape[1]
    )

    model.summary()

    # 5. Callbacks
    callbacks = [
        tf.keras.callbacks.EarlyStopping(
            patience=10,
            restore_best_weights=True,
            monitor='val_loss'
        ),
        tf.keras.callbacks.ReduceLROnPlateau(
            factor=0.5,
            patience=5,
            monitor='val_loss'
        )
    ]

    # 6. Обучение
    print(f"\n🔄 Training model...")
    history = model.fit(
        X_train_norm, y_train,
        validation_data=(X_val_norm, y_val),
        epochs=epochs,
        batch_size=batch_size,
        callbacks=callbacks,
        verbose=1
    )

    # 7. Оценка на тесте
    test_loss, test_acc = model.evaluate(X_test_norm, y_test, verbose=0)
    print(f"\n🎯 Test Accuracy: {test_acc:.4f} ({test_acc * 100:.2f}%)")

    # 8. Сохраняем модель
    model.save('gesture_model.h5')
    print(f"✅ Model saved as 'gesture_model.h5'")

    # 9. Сохраняем параметры нормализации
    np.savez('norm_params.npz', mean=mean, std=std)
    print(f"✅ Normalization params saved as 'norm_params.npz'")

    # 10. Сохраняем классы
    with open('class_names.txt', 'w') as f:
        for name in class_names:
            f.write(f"{name}\n")
    print(f"✅ Class names saved as 'class_names.txt'")

    # 11. График обучения
    plot_training(history)

    return model, history, class_names, mean, std


# ============================================================================
# 4. ВИЗУАЛИЗАЦИЯ
# ============================================================================

def plot_training(history):
    """Показывает график обучения"""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4))

    # Loss
    ax1.plot(history.history['loss'], label='Train Loss')
    ax1.plot(history.history['val_loss'], label='Val Loss')
    ax1.set_title('Loss')
    ax1.set_xlabel('Epoch')
    ax1.set_ylabel('Loss')
    ax1.legend()
    ax1.grid(True)

    # Accuracy
    ax2.plot(history.history['accuracy'], label='Train Acc')
    ax2.plot(history.history['val_accuracy'], label='Val Acc')
    ax2.set_title('Accuracy')
    ax2.set_xlabel('Epoch')
    ax2.set_ylabel('Accuracy')
    ax2.legend()
    ax2.grid(True)

    plt.tight_layout()
    plt.savefig('training_history.png')
    plt.show()


# ============================================================================
# 5. КОНВЕРТАЦИЯ В TFLite (ДЛЯ ESP32)
# ============================================================================

def convert_to_tflite(model_path, output_path='gesture_model.tflite'):
    """
    Конвертирует модель в TFLite
    """
    # Загружаем модель
    model = tf.keras.models.load_model(model_path)

    # Конвертер
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    # Сохраняем
    with open(output_path, 'wb') as f:
        f.write(tflite_model)

    print(f"✅ TFLite model saved: {output_path}")
    print(f"   Size: {len(tflite_model) / 1024:.2f} KB")


# ============================================================================
# 6. ЗАПУСК
# ============================================================================

if __name__ == "__main__":
    print("=" * 60)
    print("🚀 TRAINING GESTURE RECOGNITION MODEL")
    print("=" * 60)

    DATASET_PATH = "TEST_BIG_PACK_50_20"

    # 1. Обучение
    model, history, class_names, mean, std = train_model(
        dataset_path=DATASET_PATH,
        epochs=150,
        batch_size=32
    )

    # 2. Конвертация в TFLite
    print("\n🔄 Converting to TFLite...")

    # Конвертер
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    # Сохраняем
    with open('gesture_model.tflite', 'wb') as f:
        f.write(tflite_model)

    print(f"✅ TFLite model saved: gesture_model.tflite")
    print(f"   Size: {len(tflite_model) / 1024:.2f} KB")

    print("\n" + "=" * 60)
    print("✅ DONE! Модель готова для ESP32!")
    print("=" * 60)