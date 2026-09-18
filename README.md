flowchart TD
    A["Presenter.ino"]
    A --> B["setup()"]
    A --> Z["loop()"]

    B --> B1["Serial / WDT / I2C"]
    B --> B2["transportInit()"]
    B --> B3["setupInput()"]
    B --> B4["setupIMU()"]
    B --> B5["initInference()<br>TFLM"]
    B --> B6["inferenceTaskInit()<br>core 0"]
    B --> B7["registerTask() x4"]

    B2 --> E1{"error?"}
    B5 --> E1
    B6 --> E1
    E1 -- "fail" --> FATAL["while(1) halt"]
    B7 --> Z

    Z --> SCHED["runScheduler()"]
    SCHED --> T1["TASK_IMU<br>4 s"]
    SCHED --> T2["TASK_BUTTONS<br>15 s"]
    SCHED --> T3["TASK_GESTURES<br>4 s"]
    SCHED --> T4["TASK_SERIAL_DEBUG<br>200 s"]

    subgraph IMU["imu.cpp"]
        IMU1["updateIMU()"] --> IMU2["I2C чтение MPU-6050"] --> IMU3["масштабирование<br>+ HPF"] --> IMU4["imu.accel / gyro<br>семпл"]
    end

    subgraph INPUT["input.cpp"]
        IN1["handleButtons()"] --> IN2{"link?"}
        IN2 -- "SERIAL" --> IN3["handleRecordButton()"]
        IN2 -- "BLE_HID" --> IN4["handleButton()<br>KEY_LEFT"]
        IN1 --> IN5["UP / DOWN / RIGHT"]
        IN1 --> IN6["handleDebugMediaButton()<br>5 s → debug"]
        IN1 --> IN7["handleGestureButton()<br>toggleGesture()"]
    end

    subgraph SYS["system.cpp"]
        SYS1["workMode:<br>BUTTONS / GESTURES / DRAW"]
        SYS2["debugMode"]
    end

    subgraph GEST["gestures"]
        G1["motionHistory.addSample()"] --> G2["motionDetector.updateState()"]
        G2 --> G3{"state?"}
        G3 -- "IDLE" --> G4{"окно готово?"}
        G4 -- "yes" --> G5["→ RUNNING"]
        G3 -- "COOLDOWN" --> G6{"cooldown?"}
        G6 -- "yes" --> G5
        G3 -- "RUNNING" --> G7{"интервал?"}
        G7 -- "yes" --> G8["getWindow() →<br>inferenceRequest()"]
        G8 --> G9["takeInferenceResult()"]
        G9 --> G10{"gesture 4/5<br>+ vote?"}
        G10 -- "no" --> G11["сброс голосов"]
        G10 -- "yes" --> G12["transportSendHID()"]
        G12 --> G13["→ COOLDOWN"]
    end

    subgraph ITASK["inferenceTask.cpp"]
        IT1["inferenceTask (core 0)"] --> IT2["xSemaphoreTake(winReady)"] --> IT3["memcpy окна"] --> IT4["runInference()"] --> IT5["результат готов"]
    end

    subgraph INF["inference.cpp"]
        INF1["normalizeWindow()"] --> INF2["заполнение тензора"] --> INF3["Invoke()"] --> INF4["чтение probs"] --> INF5["argmax"] --> INF6{"maxVal > 0.8?"}
        INF6 -- "yes" --> INF7["debug: send result"] --> INF8["return class"]
        INF6 -- "no" --> INF9["return -1"]
    end

    subgraph TR["transport.cpp"]
        TR1["BleKeyboard 'Presenter'"]
        TR2["sendHID / sendMedia"]
        TR3["sendSample (recording)"]
        TR4["stopRecording"]
    end

    T1 --> IMU1
    IMU4 --> TR3
    T2 --> IN1
    T3 --> G1
    T3 --> G2
    T4 --> DBG["print gestureCount"]
    G8 --> IT2
    IT4 --> INF1
    G9 --> G10
    G12 --> TR2
    INF7 --> TR3
    IN3 --> TR3
    IN6 --> TR4
    IN6 --> SYS2
    IN7 --> SYS1